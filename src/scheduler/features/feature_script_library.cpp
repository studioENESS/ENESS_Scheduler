#include "feature_script_library.h"

#ifdef FEATURE_SCRIPT_LIBRARY

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

#include "imgui.h"
#include "imguidatechooser.h"
#include "portable-file-dialogs.h"

#include "../app_state.h"
#include "../platform.h"
#include "schedule_list_common.h"

namespace {

constexpr int kMaxScanDepth = 6;

std::mutex g_catalogMutex;
std::atomic<bool> g_workerBusy{false};
std::string g_statusMessage;
std::string g_pendingStatus;

std::shared_ptr<pfd::select_folder> g_selectFolderDialog;
std::shared_ptr<pfd::open_file> g_openContentDialog;
std::shared_ptr<pfd::open_file> g_openClientDialog;

char g_gitUrlBuf[512] = "";
char g_gitSubdirBuf[256] = "";
char g_manualNameBuf[128] = "";
int g_selectedCatalogIndex = -1;
int g_pendingApplyCatalogIndex = -1;

std::wstring DefaultPixileLocation()
{
#ifdef _WIN32
    return L"C:\\ENESS_Projects\\pixile\\Bin\\Studio\\Release\\";
#else
    return L"/home/pi/pixile/";
#endif
}

std::wstring DefaultGitReposRoot()
{
#ifdef _WIN32
    return L"C:\\Content\\repos";
#else
    return L"/home/pi/";
#endif
}

std::string Slugify(const std::string& input)
{
    std::string out;
    out.reserve(input.size());
    bool lastDash = false;
    for (unsigned char c : input)
    {
        if (std::isalnum(c))
        {
            out.push_back((char)std::tolower(c));
            lastDash = false;
        }
        else if (!lastDash)
        {
            out.push_back('-');
            lastDash = true;
        }
    }
    while (!out.empty() && out.back() == '-')
        out.pop_back();
    return out.empty() ? "script" : out;
}

bool IsProjectFolderName(const std::string& name)
{
    return name.size() >= 4
        && std::isdigit((unsigned char)name[0])
        && std::isdigit((unsigned char)name[1])
        && std::isdigit((unsigned char)name[2])
        && std::isdigit((unsigned char)name[3]);
}

bool IsIncrementalSavePxl(const std::filesystem::path& path)
{
    const std::string stem = path.stem().string();
    const auto pos = stem.rfind('_');
    if (pos == std::string::npos || pos + 1 >= stem.size())
        return false;

    const std::string suffix = stem.substr(pos + 1);
    if (suffix.empty())
        return false;

    for (char c : suffix)
    {
        if (!std::isdigit((unsigned char)c))
            return false;
    }
    return true;
}

std::string BeautifyToken(std::string text)
{
    for (char& c : text)
    {
        if (c == '_' || c == '-')
            c = ' ';
    }
    return text;
}

std::wstring DeriveDisplayName(const std::filesystem::path& pxlPath,
                               const std::filesystem::path& projectRoot = {})
{
    std::string scriptLabel = BeautifyToken(pxlPath.stem().string());
    if (projectRoot.empty())
        return utf8_decode(scriptLabel);

    std::string projectLabel = BeautifyToken(projectRoot.filename().string());
    if (scriptLabel == projectLabel)
        return utf8_decode(projectLabel);

    return utf8_decode(projectLabel + " - " + scriptLabel);
}

bool IsPiHomeScanRoot(const std::filesystem::path& root)
{
#ifndef _WIN32
    const auto normalized = root.lexically_normal().string();
    return normalized == "/home/pi";
#endif
    (void)root;
    return false;
}

bool IsClientPxl(const std::filesystem::path& path)
{
    std::string stem = path.stem().string();
    return stem.size() >= 7 && stem.compare(stem.size() - 7, 7, "_client") == 0;
}

std::filesystem::path ClientSiblingPath(const std::filesystem::path& contentPath)
{
    std::string stem = contentPath.stem().string();
    return contentPath.parent_path() / (stem + "_client.pxl");
}

bool IsValidGitUrl(const std::string& url)
{
    if (url.rfind("https://github.com/", 0) != 0)
        return false;
    if (url.find(' ') != std::string::npos)
        return false;
    return url.size() > 22;
}

std::string RepoNameFromUrl(const std::string& url)
{
    auto pos = url.rfind('/');
    if (pos == std::string::npos)
        return "repo";
    std::string name = url.substr(pos + 1);
    if (name.size() > 4 && name.substr(name.size() - 4) == ".git")
        name.resize(name.size() - 4);
    return name;
}

int DirectoryDepth(const std::filesystem::path& root, const std::filesystem::path& current)
{
    std::error_code ec;
    auto rel = std::filesystem::relative(current, root, ec);
    if (ec)
        return 0;
    return (int)std::distance(rel.begin(), rel.end());
}

void CollectPxlFromTree(const std::filesystem::path& root,
                        const std::filesystem::path& projectRoot,
                        const std::wstring& pixileLoc,
                        const std::string& sourceTag,
                        std::vector<ScriptCatalogEntry>& found)
{
    std::error_code ec;
    if (!std::filesystem::exists(root, ec))
        return;

    for (auto it = std::filesystem::recursive_directory_iterator(
             root, std::filesystem::directory_options::skip_permission_denied, ec);
         it != std::filesystem::recursive_directory_iterator(); ++it)
    {
        if (DirectoryDepth(root, it->path()) > kMaxScanDepth)
        {
            it.disable_recursion_pending();
            continue;
        }

        if (!it->is_regular_file(ec))
            continue;

        if (it->path().extension() != ".pxl")
            continue;

        if (IsIncrementalSavePxl(it->path()))
            continue;

        if (IsClientPxl(it->path()))
            continue;

        ScriptCatalogEntry entry;
        entry.content_path = utf8_decode(std::filesystem::weakly_canonical(it->path(), ec).string());
        auto clientPath = ClientSiblingPath(it->path());
        if (std::filesystem::exists(clientPath, ec) && !IsIncrementalSavePxl(clientPath))
            entry.client_path = utf8_decode(clientPath.string());

        std::string relId = std::filesystem::relative(it->path(), projectRoot, ec).string();
        entry.id = Slugify(sourceTag + "-" + relId);
        entry.name = DeriveDisplayName(it->path(), projectRoot);
        entry.pixile_location = pixileLoc;
        entry.source = sourceTag;
        found.push_back(std::move(entry));
    }
}

void ScanDirectory(const std::filesystem::path& root,
                   const std::wstring& pixileLoc,
                   const std::string& sourceTag,
                   std::vector<ScriptCatalogEntry>& found)
{
    std::error_code ec;
    if (!std::filesystem::exists(root, ec))
        return;

    if (IsPiHomeScanRoot(root))
    {
        for (const auto& entry : std::filesystem::directory_iterator(root, ec))
        {
            if (ec)
                break;
            if (!entry.is_directory(ec))
                continue;

            const std::string folderName = entry.path().filename().string();
            if (!IsProjectFolderName(folderName))
                continue;

            CollectPxlFromTree(entry.path(), entry.path(), pixileLoc, sourceTag, found);
        }
        return;
    }

    const auto projectRoot = IsProjectFolderName(root.filename().string()) ? root : std::filesystem::path{};
    CollectPxlFromTree(root, projectRoot.empty() ? root : projectRoot, pixileLoc, sourceTag, found);
}

void MergeDiscoveredEntries(std::vector<ScriptCatalogEntry>&& discovered)
{
    std::lock_guard<std::mutex> lock(g_catalogMutex);

    std::vector<ScriptCatalogEntry> merged;
    merged.reserve(g_scriptCatalog.size() + discovered.size());

    for (const auto& existing : g_scriptCatalog)
    {
        if (existing.source == "manual")
            merged.push_back(existing);
    }

    for (auto& entry : discovered)
    {
        auto it = std::find_if(merged.begin(), merged.end(), [&](const ScriptCatalogEntry& e) {
            return e.content_path == entry.content_path;
        });
        if (it != merged.end())
        {
            if (it->name_locked)
                entry.name = it->name;
            *it = std::move(entry);
        }
        else
        {
            auto prev = std::find_if(g_scriptCatalog.begin(), g_scriptCatalog.end(), [&](const ScriptCatalogEntry& e) {
                return e.content_path == entry.content_path;
            });
            if (prev != g_scriptCatalog.end() && prev->name_locked)
                entry.name = prev->name;
            merged.push_back(std::move(entry));
        }
    }

    std::sort(merged.begin(), merged.end(), [](const ScriptCatalogEntry& a, const ScriptCatalogEntry& b) {
        return utf8_encode(a.name) < utf8_encode(b.name);
    });

    g_scriptCatalog = std::move(merged);
    g_pendingStatus = "Found " + std::to_string(g_scriptCatalog.size()) + " script(s).";
}

void RunScanWorker()
{
    std::vector<ScriptCatalogEntry> discovered;
    const auto pixileLoc = DefaultPixileLocation();

    for (const auto& root : g_scanRoots)
    {
        if (!root.enabled)
            continue;
        ScanDirectory(std::filesystem::path(utf8_encode(root.path)), pixileLoc, "scan", discovered);
    }

    for (const auto& repo : g_gitRepos)
    {
        std::filesystem::path projectRoot = std::filesystem::path(utf8_encode(repo.clone_path));
        std::filesystem::path base = projectRoot;
        if (!repo.search_subdir.empty())
            base /= utf8_encode(repo.search_subdir);

        if (IsPiHomeScanRoot(base))
            ScanDirectory(base, pixileLoc, "git", discovered);
        else
            CollectPxlFromTree(base, projectRoot, pixileLoc, "git", discovered);
    }

    std::sort(discovered.begin(), discovered.end(), [](const ScriptCatalogEntry& a, const ScriptCatalogEntry& b) {
        const auto pathCmp = utf8_encode(a.content_path).compare(utf8_encode(b.content_path));
        if (pathCmp != 0)
            return pathCmp < 0;
        return utf8_encode(a.name) < utf8_encode(b.name);
    });

    MergeDiscoveredEntries(std::move(discovered));
}

void StartBackgroundTask(const std::function<void()>& task)
{
    if (g_workerBusy.exchange(true))
        return;

    std::thread([task]() {
        task();
        g_workerBusy = false;
    }).detach();
}

bool RunShellCommand(const std::string& cmd)
{
    int rc = std::system(cmd.c_str());
    return rc == 0;
}

void CreateDefaultLibraryFile()
{
    g_scanRoots.clear();
#ifdef _WIN32
    g_scanRoots.push_back({L"C:\\Content", true});
#else
    g_scanRoots.push_back({L"/home/pi/", true});
#endif
    g_gitRepos.clear();
    g_scriptCatalog.clear();
    ScriptLibrary_Save();
}

} // namespace

std::vector<ScriptCatalogEntry> g_scriptCatalog;
std::vector<ScanRoot> g_scanRoots;
std::vector<GitRepoSource> g_gitRepos;
bool g_gitAvailable = false;

void ScriptLibrary_Init()
{
    g_gitAvailable = RunShellCommand("git --version >nul 2>&1");
#ifndef _WIN32
    if (!g_gitAvailable)
        g_gitAvailable = RunShellCommand("git --version >/dev/null 2>&1");
#endif

    std::ifstream file(SCHEDULER_SCRIPT_LIBRARY_PATH);
    if (!file.is_open())
    {
        CreateDefaultLibraryFile();
        g_statusMessage = "Created default script library.";
        return;
    }

    try
    {
        nlohmann::json jsonfile;
        file >> jsonfile;

        g_scanRoots.clear();
        if (jsonfile.contains("scan_roots"))
        {
            for (const auto& root : jsonfile["scan_roots"])
            {
                ScanRoot item;
                item.path = utf8_decode(root.value("path", ""));
                item.enabled = root.value("enabled", true);
                if (!item.path.empty())
                    g_scanRoots.push_back(item);
            }
        }

        g_gitRepos.clear();
        if (jsonfile.contains("git_repos"))
        {
            for (const auto& repo : jsonfile["git_repos"])
            {
                GitRepoSource item;
                item.url = repo.value("url", "");
                item.clone_path = utf8_decode(repo.value("clone_path", ""));
                item.search_subdir = utf8_decode(repo.value("search_subdir", ""));
                if (!item.url.empty())
                    g_gitRepos.push_back(item);
            }
        }

        g_scriptCatalog.clear();
        if (jsonfile.contains("catalog"))
        {
            for (const auto& entry : jsonfile["catalog"])
            {
                ScriptCatalogEntry item;
                item.id = entry.value("id", "");
                item.name = utf8_decode(entry.value("name", ""));
                item.content_path = utf8_decode(entry.value("content_path", ""));
                item.client_path = utf8_decode(entry.value("client_path", ""));
                item.pixile_location = utf8_decode(entry.value("pixile_location", ""));
                item.source = entry.value("source", "scan");
                item.name_locked = entry.value("name_locked", item.source == "manual");
                if (!item.id.empty() && !item.content_path.empty())
                    g_scriptCatalog.push_back(item);
            }
        }
    }
    catch (const std::exception& e)
    {
        g_statusMessage = std::string("Library load error: ") + e.what();
    }
}

void ScriptLibrary_Save()
{
    nlohmann::json jsonfile;

    for (const auto& root : g_scanRoots)
    {
        jsonfile["scan_roots"].push_back({
            {"path", utf8_encode(root.path)},
            {"enabled", root.enabled},
        });
    }

    for (const auto& repo : g_gitRepos)
    {
        jsonfile["git_repos"].push_back({
            {"url", repo.url},
            {"clone_path", utf8_encode(repo.clone_path)},
            {"search_subdir", utf8_encode(repo.search_subdir)},
        });
    }

    for (const auto& entry : g_scriptCatalog)
    {
        jsonfile["catalog"].push_back({
            {"id", entry.id},
            {"name", utf8_encode(entry.name)},
            {"content_path", utf8_encode(entry.content_path)},
            {"client_path", utf8_encode(entry.client_path)},
            {"pixile_location", utf8_encode(entry.pixile_location)},
            {"source", entry.source},
            {"name_locked", entry.name_locked},
        });
    }

    std::filesystem::path path(SCHEDULER_SCRIPT_LIBRARY_PATH);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream outfile(SCHEDULER_SCRIPT_LIBRARY_PATH, std::ios::out | std::ios::trunc);
    outfile << jsonfile.dump(4);
}

void ScriptLibrary_PollFileDialogs()
{
    if (!g_pendingStatus.empty() && !g_workerBusy)
    {
        g_statusMessage = g_pendingStatus;
        g_pendingStatus.clear();
        ScriptLibrary_Save();
    }

    if (g_selectFolderDialog && g_selectFolderDialog->ready())
    {
        auto result = g_selectFolderDialog->result();
        if (!result.empty())
        {
            ScanRoot root;
            root.path = utf8_decode(result[0]);
            root.enabled = true;
            g_scanRoots.push_back(root);
            ScriptLibrary_Save();
        }
        g_selectFolderDialog = nullptr;
    }

    if (g_openContentDialog && g_openContentDialog->ready())
    {
        auto result = g_openContentDialog->result();
        if (!result.empty())
        {
            std::filesystem::path p(result[0]);
            ScriptCatalogEntry entry;
            entry.content_path = utf8_decode(result[0]);
            entry.id = Slugify("manual-" + p.filename().string());
            entry.name = DeriveDisplayName(p);
            entry.pixile_location = DefaultPixileLocation();
            entry.source = "manual";
            entry.name_locked = true;

            auto clientPath = ClientSiblingPath(p);
            std::error_code ec;
            if (std::filesystem::exists(clientPath, ec))
                entry.client_path = utf8_decode(clientPath.string());

            if (g_manualNameBuf[0])
                entry.name = utf8_decode(std::string(g_manualNameBuf));

            g_scriptCatalog.push_back(entry);
            ScriptLibrary_Save();
        }
        g_openContentDialog = nullptr;
    }

    if (g_openClientDialog && g_openClientDialog->ready())
    {
        auto result = g_openClientDialog->result();
        if (!result.empty() && g_selectedCatalogIndex >= 0 && g_selectedCatalogIndex < (int)g_scriptCatalog.size())
        {
            g_scriptCatalog[g_selectedCatalogIndex].client_path = utf8_decode(result[0]);
            ScriptLibrary_Save();
        }
        g_openClientDialog = nullptr;
    }
}

const ScriptCatalogEntry* ScriptLibrary_FindEntry(const std::string& id)
{
    if (id.empty())
        return nullptr;
    auto it = std::find_if(g_scriptCatalog.begin(), g_scriptCatalog.end(), [&](const ScriptCatalogEntry& e) {
        return e.id == id;
    });
    return it != g_scriptCatalog.end() ? &(*it) : nullptr;
}

int ScriptLibrary_FindEntryIndex(const std::string& id)
{
    for (int i = 0; i < (int)g_scriptCatalog.size(); ++i)
        if (g_scriptCatalog[i].id == id)
            return i;
    return -1;
}

bool ScriptLibrary_ApplyEntry(const std::string& id)
{
    const auto* entry = ScriptLibrary_FindEntry(id);
    if (!entry)
        return false;

    content_filename = entry->content_path;
    orig_content_filename = entry->content_path;
    client_filename = entry->client_path;
    if (!entry->pixile_location.empty())
        pixile_location = entry->pixile_location;
    return true;
}

void ScriptLibrary_LoadSchedule(nlohmann::json& jsonfile)
{
    for (auto item : g_vecSchedule)
        delete item;
    g_vecSchedule.clear();

    if (!jsonfile.contains("schedule"))
        return;

    for (auto& item : jsonfile["schedule"])
    {
        if (item.is_null())
            continue;

        auto newItem = new SItemSchedule;
        newItem->index = (int32_t)g_vecSchedule.size();
        ImGui::SetDateToday(&newItem->startDate);
        ImGui::SetDateToday(&newItem->endDate);
        newItem->index = item.value("index", newItem->index);
        newItem->startDate.tm_year = item["StartDate"]["Year"];
        newItem->startDate.tm_mon = item["StartDate"]["Month"];
        newItem->startDate.tm_mday = item["StartDate"]["Day"];
        newItem->startDate.tm_yday = item["StartDate"]["YearDay"];
        newItem->endDate.tm_year = item["EndDate"]["Year"];
        newItem->endDate.tm_mon = item["EndDate"]["Month"];
        newItem->endDate.tm_mday = item["EndDate"]["Day"];
        newItem->endDate.tm_yday = item["EndDate"]["YearDay"];
        if (item.contains("CatalogID"))
            newItem->catalog_id = item["CatalogID"];

        g_vecSchedule.push_back(newItem);
    }

    std::sort(g_vecSchedule.begin(), g_vecSchedule.end(), compareByStartDate);
}

void ScriptLibrary_SaveSchedule(nlohmann::json& jsonfile)
{
    int index = 0;
    for (const auto sched : g_vecSchedule)
    {
        jsonfile["schedule"][index]["index"] = index;
        jsonfile["schedule"][index]["StartDate"]["Year"] = sched->startDate.tm_year;
        jsonfile["schedule"][index]["StartDate"]["Month"] = sched->startDate.tm_mon;
        jsonfile["schedule"][index]["StartDate"]["Day"] = sched->startDate.tm_mday;
        jsonfile["schedule"][index]["StartDate"]["YearDay"] = sched->startDate.tm_yday;
        jsonfile["schedule"][index]["EndDate"]["Year"] = sched->endDate.tm_year;
        jsonfile["schedule"][index]["EndDate"]["Month"] = sched->endDate.tm_mon;
        jsonfile["schedule"][index]["EndDate"]["Day"] = sched->endDate.tm_mday;
        jsonfile["schedule"][index]["EndDate"]["YearDay"] = sched->endDate.tm_yday;
        jsonfile["schedule"][index]["CatalogID"] = sched->catalog_id;
        index++;
    }
}

bool ScriptLibrary_ScriptChanged(int item, int lastItem)
{
    if (item < 0 || lastItem < 0)
        return item != lastItem;
    if (item >= (int)g_vecSchedule.size() || lastItem >= (int)g_vecSchedule.size())
        return true;
    return g_vecSchedule[item]->catalog_id != g_vecSchedule[lastItem]->catalog_id;
}

void ScriptLibrary_DrawCurrentContent(int item)
{
    ImGui::Text("Current Scheduled Content:");
    ImGui::SameLine();
    if (item >= 0 && item < (int)g_vecSchedule.size())
    {
        const auto* entry = ScriptLibrary_FindEntry(g_vecSchedule[item]->catalog_id);
        if (entry)
            ImGui::Text("%s", utf8_encode(entry->name).c_str());
        else
            ImGui::TextDisabled("(no script selected)");
    }
    else
    {
        ImGui::TextDisabled("(no active schedule item)");
    }
}

void ScriptLibrary_DrawScheduleUI()
{
    DrawScheduleListUI(
        [](SItemSchedule* item) {
            const auto* entry = ScriptLibrary_FindEntry(item->catalog_id);
            return entry ? utf8_encode(entry->name) : std::string("(unassigned)");
        },
        [](SItemSchedule* item) {
            const auto* current = ScriptLibrary_FindEntry(item->catalog_id);
            const char* preview = current ? utf8_encode(current->name).c_str() : "(select script)";
            ImGui::SetNextItemWidth(220);
            if (ImGui::BeginCombo("Script", preview))
            {
                for (const auto& entry : g_scriptCatalog)
                {
                    const bool is_selected = item->catalog_id == entry.id;
                    if (ImGui::Selectable(utf8_encode(entry.name).c_str(), is_selected))
                        item->catalog_id = entry.id;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        });
}

void ScriptLibrary_DrawUI()
{
    if (ImGui::CollapsingHeader("Script Library", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (!g_gitAvailable)
            ImGui::TextColored(ImColor(IM_COL32(255, 200, 0, 255)), "git not found on PATH — install git to clone repos.");

        if (!g_statusMessage.empty())
            ImGui::TextWrapped("%s", g_statusMessage.c_str());

        const bool busy = g_workerBusy.load();

        if (busy)
            ImGui::BeginDisabled();
        if (ImGui::Button("Scan Now"))
        {
            g_statusMessage = "Scanning...";
            StartBackgroundTask(RunScanWorker);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Library"))
            ScriptLibrary_Save();
        if (busy)
            ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::Text("Catalog (%d)", (int)g_scriptCatalog.size());

        if (ImGui::BeginTable("ScriptCatalog", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 120)))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Content");
            ImGui::TableSetupColumn("Client");
            ImGui::TableSetupColumn("Source");
            ImGui::TableHeadersRow();

            for (int i = 0; i < (int)g_scriptCatalog.size(); ++i)
            {
                const auto& entry = g_scriptCatalog[i];
                ImGui::TableNextRow();
                ImGui::PushID(i);

                ImGui::TableSetColumnIndex(0);
                bool selected = g_selectedCatalogIndex == i;
                if (ImGui::Selectable(utf8_encode(entry.name).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                    g_selectedCatalogIndex = i;

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(utf8_encode(entry.content_path).c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(entry.client_path.empty() ? "-" : utf8_encode(entry.client_path).c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(entry.source.c_str());

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if (g_selectedCatalogIndex >= 0 && g_selectedCatalogIndex < (int)g_scriptCatalog.size())
        {
            if (ImGui::Button("Apply as Active"))
            {
                g_pendingApplyCatalogIndex = g_selectedCatalogIndex;
            }
            ImGui::SameLine();
            if (ImGui::Button("Set Client .pxl..."))
            {
                std::vector<std::string> filters = {"Pixile Script", "*.pxl"};
                g_openClientDialog = std::make_shared<pfd::open_file>("Choose client script", utf8_encode(g_scriptCatalog[g_selectedCatalogIndex].content_path), filters);
            }
        }

        if (g_pendingApplyCatalogIndex >= 0 && g_pendingApplyCatalogIndex < (int)g_scriptCatalog.size())
        {
            ScriptLibrary_ApplyEntry(g_scriptCatalog[g_pendingApplyCatalogIndex].id);
            g_pendingApplyCatalogIndex = -1;
        }

        ImGui::Separator();
        ImGui::Text("Scan Roots");
        for (int i = 0; i < (int)g_scanRoots.size(); ++i)
        {
            ImGui::PushID(i);
            ImGui::Checkbox("##enabled", &g_scanRoots[i].enabled);
            ImGui::SameLine();
            ImGui::TextUnformatted(utf8_encode(g_scanRoots[i].path).c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove"))
            {
                g_scanRoots.erase(g_scanRoots.begin() + i);
                ScriptLibrary_Save();
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }

        if (ImGui::Button("Add Scan Folder..."))
        {
#ifdef _WIN32
            g_selectFolderDialog = std::make_shared<pfd::select_folder>("Choose scan folder", "C:\\");
#else
            g_selectFolderDialog = std::make_shared<pfd::select_folder>("Choose scan folder", "/home/pi");
#endif
        }

        ImGui::Separator();
        ImGui::Text("Git Repos (public github.com HTTPS only)");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##giturl", g_gitUrlBuf, sizeof(g_gitUrlBuf));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##gitsubdir", "search subdir (e.g. Pixile_Sketch/Packed)", g_gitSubdirBuf, sizeof(g_gitSubdirBuf));

        if (busy)
            ImGui::BeginDisabled();
        if (ImGui::Button("Add && Clone Repo"))
        {
            std::string url = g_gitUrlBuf;
            if (!IsValidGitUrl(url))
            {
                g_statusMessage = "URL must start with https://github.com/";
            }
            else if (!g_gitAvailable)
            {
                g_statusMessage = "git is not available on this system.";
            }
            else
            {
                GitRepoSource repo;
                repo.url = url;
                repo.search_subdir = utf8_decode(g_gitSubdirBuf);
                repo.clone_path = DefaultGitReposRoot();
                if (!repo.clone_path.empty() && repo.clone_path.back() != L'/' && repo.clone_path.back() != L'\\')
                    repo.clone_path += L"/";
                repo.clone_path += utf8_decode(RepoNameFromUrl(url));

                std::string clonePath = utf8_encode(repo.clone_path);
                std::error_code ec;
                g_gitRepos.push_back(repo);
                ScriptLibrary_Save();
                if (!std::filesystem::exists(repo.clone_path, ec))
                {
                    std::string cmd = "git clone --depth 1 \"" + url + "\" \"" + clonePath + "\"";
                    g_statusMessage = "Cloning " + url + "...";
                    StartBackgroundTask([cmd]() {
                        RunShellCommand(cmd);
                        RunScanWorker();
                    });
                }
                else
                {
                    g_statusMessage = "Repo path already exists; rescanning.";
                    StartBackgroundTask(RunScanWorker);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Pull && Rescan All"))
        {
            if (!g_gitAvailable)
                g_statusMessage = "git is not available on this system.";
            else
            {
                g_statusMessage = "Pulling repos...";
                StartBackgroundTask([]() {
                    for (const auto& repo : g_gitRepos)
                    {
                        std::string cmd = "git -C \"" + utf8_encode(repo.clone_path) + "\" pull --ff-only";
                        RunShellCommand(cmd);
                    }
                    RunScanWorker();
                });
            }
        }
        if (busy)
            ImGui::EndDisabled();

        for (int i = 0; i < (int)g_gitRepos.size(); ++i)
        {
            ImGui::BulletText("%s -> %s", g_gitRepos[i].url.c_str(), utf8_encode(g_gitRepos[i].clone_path).c_str());
        }

        ImGui::Separator();
        ImGui::Text("Manual Add");
        ImGui::SetNextItemWidth(180);
        ImGui::InputTextWithHint("##manualname", "display name", g_manualNameBuf, sizeof(g_manualNameBuf));
        ImGui::SameLine();
        if (ImGui::Button("Browse content .pxl..."))
        {
            std::vector<std::string> filters = {"Pixile Script", "*.pxl"};
#ifdef _WIN32
            g_openContentDialog = std::make_shared<pfd::open_file>("Choose content script", "C:\\", filters);
#else
            g_openContentDialog = std::make_shared<pfd::open_file>("Choose content script", "/home/pi", filters);
#endif
        }
    }
}

int ScriptLibrary_AdjustWindowHeight(int height)
{
    return height + 420;
}

#endif // FEATURE_SCRIPT_LIBRARY
