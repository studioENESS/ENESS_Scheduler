import json
import os
import sys
import datetime
import subprocess
import threading
import time
import signal
import paramiko
from enum import Enum
import imgui
import glfw
from imgui.integrations.glfw import GlfwRenderer

# Constants
APP_NAME = "IMGUI Scheduler"
CONFIG_FILE = "scheduler_config.json"
DAYS_OF_WEEK = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]
TIME_FORMAT = "%H:%M"
LOGS_DIR = "scheduler_logs"

class TaskType(Enum):
    EXECUTABLE = 0
    SCRIPT = 1
    SSH_COMMAND = 2


class ScheduledTask:
    def __init__(self, name="Task", task_type=TaskType.SCRIPT, path="", 
                 time="12:00", days_of_week=None, start_date="", end_date="", enabled=True,
                 ssh_host="", ssh_port=22, ssh_username="", ssh_password="", ssh_key_path="",
                 timeout=60, retry_count=0, notify_on_failure=True, log_output=True):
        self.name = name
        self.task_type = task_type
        self.path = path
        self.time = time
        self.days_of_week = days_of_week if days_of_week else [False] * 7
        self.start_date = start_date
        self.end_date = end_date
        self.enabled = enabled
        self.last_run = None
        self.last_status = None
        self.is_running = False
        self.process = None
        
        # SSH specific properties
        self.ssh_host = ssh_host
        self.ssh_port = ssh_port
        self.ssh_username = ssh_username
        self.ssh_password = ssh_password
        self.ssh_key_path = ssh_key_path
        
        # Error handling properties
        self.timeout = timeout  # seconds before considering a task as hung
        self.retry_count = retry_count  # number of retries if task fails
        self.retry_attempts = 0  # current retry count
        self.notify_on_failure = notify_on_failure
        self.log_output = log_output
    
    def to_dict(self):
        return {
            "name": self.name,
            "task_type": self.task_type.value,
            "path": self.path,
            "time": self.time,
            "days_of_week": self.days_of_week,
            "start_date": self.start_date,
            "end_date": self.end_date,
            "enabled": self.enabled,
            "ssh_host": self.ssh_host,
            "ssh_port": self.ssh_port,
            "ssh_username": self.ssh_username,
            "ssh_password": self.ssh_password,  # Note: In production, consider encrypting this
            "ssh_key_path": self.ssh_key_path,
            "timeout": self.timeout,
            "retry_count": self.retry_count,
            "notify_on_failure": self.notify_on_failure,
            "log_output": self.log_output
        }
    
    @classmethod
    def from_dict(cls, data):
        return cls(
            name=data.get("name", "Task"),
            task_type=TaskType(data.get("task_type", 0)),
            path=data.get("path", ""),
            time=data.get("time", "12:00"),
            days_of_week=data.get("days_of_week", [False] * 7),
            start_date=data.get("start_date", ""),
            end_date=data.get("end_date", ""),
            enabled=data.get("enabled", True),
            ssh_host=data.get("ssh_host", ""),
            ssh_port=data.get("ssh_port", 22),
            ssh_username=data.get("ssh_username", ""),
            ssh_password=data.get("ssh_password", ""),
            ssh_key_path=data.get("ssh_key_path", ""),
            timeout=data.get("timeout", 60),
            retry_count=data.get("retry_count", 0),
            notify_on_failure=data.get("notify_on_failure", True),
            log_output=data.get("log_output", True)
        )
    
    def should_run(self, current_time):
        # If task is disabled, don't run
        if not self.enabled:
            return False
        
        # If task is currently running, don't start again
        if self.is_running:
            return False
            
        # If task was already run in the last minute, don't run again
        if (self.last_run and 
            (datetime.datetime.now() - self.last_run).total_seconds() < 60):
            return False
            
        # Check if current day is in the scheduled days
        current_day = datetime.datetime.now().weekday()  # 0 is Monday
        if not self.days_of_week[current_day]:
            return False
            
        # Check time
        task_hour, task_minute = map(int, self.time.split(':'))
        current_hour, current_minute = current_time.hour, current_time.minute
        
        if task_hour != current_hour or task_minute != current_minute:
            return False
            
        # Check date range if set
        current_date = datetime.date.today()
        if self.start_date:
            try:
                start = datetime.datetime.strptime(self.start_date, "%Y-%m-%d").date()
                if current_date < start:
                    return False
            except ValueError:
                return False
                
        if self.end_date:
            try:
                end = datetime.datetime.strptime(self.end_date, "%Y-%m-%d").date()
                if current_date > end:
                    return False
            except ValueError:
                return False
                
        return True
        
    def run(self):
        # Create logs directory if it doesn't exist
        if self.log_output and not os.path.exists(LOGS_DIR):
            os.makedirs(LOGS_DIR)
            
        log_file = None
        if self.log_output:
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            log_filename = f"{LOGS_DIR}/{self.name.replace(' ', '_')}_{timestamp}.log"
            log_file = open(log_filename, "w")
            
        self.is_running = True
        self.last_run = datetime.datetime.now()
        self.retry_attempts = 0
        
        try:
            if self.task_type == TaskType.SSH_COMMAND:
                self._run_ssh_command(log_file)
            else:
                self._run_local_command(log_file)
                
            self.last_status = "Success"
            return True
            
        except Exception as e:
            error_msg = f"Error running task '{self.name}': {e}"
            print(error_msg)
            self.last_status = f"Failed: {str(e)}"
            
            if log_file:
                log_file.write(f"{error_msg}\n")
                
            # Handle retry logic
            if self.retry_attempts < self.retry_count:
                self.retry_attempts += 1
                print(f"Retrying task '{self.name}' (Attempt {self.retry_attempts}/{self.retry_count})")
                time.sleep(2)  # Wait before retrying
                return self.run()
                
            return False
            
        finally:
            self.is_running = False
            if log_file:
                log_file.close()
                
    def _run_local_command(self, log_file=None):
        # Prepare command based on task type
        if self.task_type == TaskType.EXECUTABLE:
            cmd = self.path
            shell = True
        else:  # SCRIPT
            if self.path.endswith('.py'):
                cmd = [sys.executable, self.path]
            elif self.path.endswith('.sh'):
                cmd = ['bash', self.path]
            else:
                cmd = self.path
                shell = True
                
        # Run the command
        if isinstance(cmd, list):
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE if log_file else None,
                stderr=subprocess.PIPE if log_file else None,
                universal_newlines=True,
                shell=False
            )
        else:
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE if log_file else None,
                stderr=subprocess.PIPE if log_file else None,
                universal_newlines=True,
                shell=True
            )
            
        self.process = process
        
        # Set up a timer to handle process timeout
        timer = None
        if self.timeout > 0:
            def kill_on_timeout():
                if process.poll() is None:  # If process is still running
                    try:
                        if os.name == 'nt':  # Windows
                            process.kill()
                        else:  # Linux/macOS
                            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
                    except:
                        pass
                    if log_file:
                        log_file.write(f"Process timed out after {self.timeout} seconds and was terminated.\n")
                        
            timer = threading.Timer(self.timeout, kill_on_timeout)
            timer.start()
            
        # Capture output if logging is enabled
        if log_file:
            while True:
                output = process.stdout.readline()
                if output == '' and process.poll() is not None:
                    break
                if output:
                    log_file.write(output)
                    log_file.flush()
                    
            stderr_output = process.stderr.read()
            if stderr_output:
                log_file.write(f"--- STDERR ---\n{stderr_output}\n")
                
        # Wait for process to complete
        exit_code = process.wait()
        
        # Cancel the timeout timer if it's still active
        if timer and timer.is_alive():
            timer.cancel()
            
        # Handle non-zero exit code
        if exit_code != 0:
            raise Exception(f"Process exited with code {exit_code}")
            
        self.process = None
                
    def _run_ssh_command(self, log_file=None):
        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        
        try:
            # Connect with password or key
            if self.ssh_key_path:
                private_key = paramiko.RSAKey.from_private_key_file(self.ssh_key_path)
                client.connect(
                    self.ssh_host,
                    port=self.ssh_port,
                    username=self.ssh_username,
                    pkey=private_key,
                    timeout=10
                )
            else:
                client.connect(
                    self.ssh_host,
                    port=self.ssh_port,
                    username=self.ssh_username,
                    password=self.ssh_password,
                    timeout=10
                )
                
            # Execute command
            if log_file:
                log_file.write(f"Executing SSH command on {self.ssh_host}:\n{self.path}\n\n")
                
            stdin, stdout, stderr = client.exec_command(
                self.path, 
                timeout=self.timeout
            )
            
            # Capture and log output
            if log_file:
                for line in stdout:
                    log_file.write(line)
                    
                stderr_output = stderr.read().decode()
                if stderr_output:
                    log_file.write(f"--- STDERR ---\n{stderr_output}\n")
                    
            # Check exit status
            exit_status = stdout.channel.recv_exit_status()
            if exit_status != 0:
                raise Exception(f"SSH command exited with code {exit_status}")
                
        finally:
            client.close()
            
    def stop(self):
        """Stop the running task if possible"""
        if not self.is_running or not self.process:
            return False
            
        try:
            if os.name == 'nt':  # Windows
                self.process.kill()
            else:  # Linux/macOS
                os.killpg(os.getpgid(self.process.pid), signal.SIGTERM)
            self.process = None
            self.is_running = False
            self.last_status = "Stopped by user"
            return True
        except:
            return False


class SchedulerApp:
    def __init__(self):
        self.tasks = []
        self.selected_task = -1
        self.new_task = ScheduledTask()
        self.show_add_task = False
        self.scheduler_thread = None
        self.running = True
        self.task_runner_active = True
        self.show_ssh_options = False
        self.show_advanced_options = False
        self.show_logs_window = False
        self.log_entries = []
        self.max_log_entries = 100
        
        # GUI state variables
        self.hour = 12
        self.minute = 0
        self.date_buffer_start = [0] * 10  # YYYY-MM-DD
        self.date_buffer_end = [0] * 10    # YYYY-MM-DD
        
        # Load saved configuration
        self.load_config()
        
        # Start the scheduler thread
        self.scheduler_thread = threading.Thread(target=self.task_runner)
        self.scheduler_thread.daemon = True
        self.scheduler_thread.start()
        
    def load_config(self):
        try:
            if os.path.exists(CONFIG_FILE):
                with open(CONFIG_FILE, 'r') as f:
                    data = json.load(f)
                    self.tasks = [ScheduledTask.from_dict(task) for task in data.get('tasks', [])]
                    self.add_log(f"Loaded {len(self.tasks)} tasks from configuration file")
            else:
                self.add_log("No configuration file found. Starting with empty task list")
        except Exception as e:
            self.add_log(f"Error loading config: {e}")
            
    def save_config(self):
        try:
            data = {
                'tasks': [task.to_dict() for task in self.tasks]
            }
            with open(CONFIG_FILE, 'w') as f:
                json.dump(data, f, indent=2)
            self.add_log(f"Saved {len(self.tasks)} tasks to configuration file")
        except Exception as e:
            self.add_log(f"Error saving config: {e}")
            
    def add_log(self, message):
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.log_entries.append(f"[{timestamp}] {message}")
        if len(self.log_entries) > self.max_log_entries:
            self.log_entries.pop(0)
        print(f"[{timestamp}] {message}")
            
    def task_runner(self):
        self.add_log("Task scheduler thread started")
        while self.task_runner_active:
            current_time = datetime.datetime.now()
            
            for task in self.tasks:
                if task.should_run(current_time):
                    self.add_log(f"Running task: {task.name}")
                    
                    # Run the task in a separate thread to avoid blocking the scheduler
                    task_thread = threading.Thread(target=self._run_task_wrapper, args=(task,))
                    task_thread.daemon = True
                    task_thread.start()
            
            # Sleep until the next minute
            sleep_time = 60 - current_time.second
            time.sleep(sleep_time)
            
    def _run_task_wrapper(self, task):
        """Wrapper to run a task and handle its status"""
        success = task.run()
        status = "successfully" if success else "with errors"
        self.add_log(f"Task '{task.name}' completed {status}")
            
    def setup_gui(self):
        # Initialize GLFW
        if not glfw.init():
            self.add_log("Could not initialize GLFW")
            return False
            
        # Create window
        window = glfw.create_window(900, 700, APP_NAME, None, None)
        if not window:
            glfw.terminate()
            self.add_log("Could not create window")
            return False
            
        glfw.make_context_current(window)
        
        # Initialize ImGui
        imgui.create_context()
        impl = GlfwRenderer(window)
        
        io = imgui.get_io()
        io.fonts.add_font_default()
        
        # Main loop
        while not glfw.window_should_close(window) and self.running:
            glfw.poll_events()
            impl.process_inputs()
            
            imgui.new_frame()
            
            self.render_ui()
            
            imgui.render()
            
            glfw.make_context_current(window)
            width, height = glfw.get_framebuffer_size(window)
            glfw.get_framebuffer_size(window)
            
            # glfw.viewport(0, 0, width, height)
            #glfw.clear_color(0.1, 0.1, 0.1, 1)
            #glfw.clear(glfw.COLOR_BUFFER_BIT)
            
            impl.render(imgui.get_draw_data())
            glfw.swap_buffers(window)
            
        # Cleanup
        impl.shutdown()
        glfw.terminate()
        self.task_runner_active = False
        
    def render_ui(self):
        imgui.begin("Scheduler Tasks", True)
        
        # Add task button
        if imgui.button("Add Task"):
            self.show_add_task = True
            self.new_task = ScheduledTask()
            # Initialize date buffers
            today = datetime.date.today().strftime("%Y-%m-%d")
            self.date_buffer_start = list(today) + ['\0']
            self.date_buffer_end = list(today) + ['\0']
        
        imgui.same_line()
        if imgui.button("Delete Selected Task") and self.selected_task >= 0 and self.selected_task < len(self.tasks):
            task = self.tasks[self.selected_task]
            if task.is_running:
                task.stop()
            self.tasks.pop(self.selected_task)
            self.selected_task = -1
            self.save_config()
            
        imgui.same_line()
        if imgui.button("Show Logs"):
            self.show_logs_window = not self.show_logs_window
            
        imgui.separator()
        
        # Tasks list
        if len(self.tasks) == 0:
            imgui.text("No tasks scheduled. Click 'Add Task' to create one.")
        else:
            imgui.begin_child("TasksList", 0, 300, border=True)
            for i, task in enumerate(self.tasks):
                status = "Running" if task.is_running else ("Enabled" if task.enabled else "Disabled")
                task_type = "SSH" if task.task_type == TaskType.SSH_COMMAND else ("Executable" if task.task_type == TaskType.EXECUTABLE else "Script")
                label = f"{task.name} - {task.time} ({status}, {task_type})"
                
                if task.last_status:
                    label += f" | Last run: {task.last_status}"
                
                if imgui.selectable(label, self.selected_task == i)[0]:
                    self.selected_task = i
                    
            imgui.end_child()
            
            # Show selected task details
            if self.selected_task >= 0 and self.selected_task < len(self.tasks):
                task = self.tasks[self.selected_task]
                imgui.separator()
                imgui.text("Task Details:")
                imgui.text(f"Name: {task.name}")
                task_type_str = {
                    TaskType.EXECUTABLE: "Executable",
                    TaskType.SCRIPT: "Script",
                    TaskType.SSH_COMMAND: "SSH Command"
                }.get(task.task_type, "Unknown")
                imgui.text(f"Type: {task_type_str}")
                imgui.text(f"Path/Command: {task.path}")
                imgui.text(f"Time: {task.time}")
                
                # Days of week
                days_str = ", ".join([DAYS_OF_WEEK[i] for i, enabled in enumerate(task.days_of_week) if enabled])
                imgui.text(f"Days: {days_str if days_str else 'None'}")
                
                if task.start_date:
                    imgui.text(f"Start Date: {task.start_date}")
                if task.end_date:
                    imgui.text(f"End Date: {task.end_date}")
                    
                # SSH details if applicable
                if task.task_type == TaskType.SSH_COMMAND:
                    imgui.text(f"SSH Host: {task.ssh_host}:{task.ssh_port}")
                    imgui.text(f"SSH User: {task.ssh_username}")
                    if task.ssh_key_path:
                        imgui.text(f"Using SSH Key: {task.ssh_key_path}")
                
                # Status info
                if task.last_run:
                    imgui.text(f"Last Run: {task.last_run.strftime('%Y-%m-%d %H:%M:%S')}")
                if task.last_status:
                    imgui.text(f"Status: {task.last_status}")
                    
                # Toggle enabled
                changed, task.enabled = imgui.checkbox("Enabled", task.enabled)
                if changed:
                    self.save_config()
                    
                # Action buttons
                if task.is_running:
                    if imgui.button("Stop Task"):
                        task.stop()
                else:
                    if imgui.button("Run Now"):
                        thread = threading.Thread(target=self._run_task_wrapper, args=(task,))
                        thread.daemon = True
                        thread.start()
                        
                imgui.same_line()
                # Edit button
                if imgui.button("Edit Task"):
                    self.new_task = ScheduledTask(
                        name=task.name,
                        task_type=task.task_type,
                        path=task.path,
                        time=task.time,
                        days_of_week=task.days_of_week.copy(),
                        start_date=task.start_date,
                        end_date=task.end_date,
                        enabled=task.enabled,
                        ssh_host=task.ssh_host,
                        ssh_port=task.ssh_port,
                        ssh_username=task.ssh_username,
                        ssh_password=task.ssh_password,
                        ssh_key_path=task.ssh_key_path,
                        timeout=task.timeout,
                        retry_count=task.retry_count,
                        notify_on_failure=task.notify_on_failure,
                        log_output=task.log_output
                    )
                    if task.start_date:
                        self.date_buffer_start = list(task.start_date) + ['\0'] * (10 - len(task.start_date))
                    else:
                        self.date_buffer_start = list(datetime.date.today().strftime("%Y-%m-%d")) + ['\0']
                        
                    if task.end_date:
                        self.date_buffer_end = list(task.end_date) + ['\0'] * (10 - len(task.end_date))
                    else:
                        self.date_buffer_end = list(datetime.date.today().strftime("%Y-%m-%d")) + ['\0']
                        
                    # Extract time
                    try:
                        self.hour, self.minute = map(int, task.time.split(':'))
                    except:
                        self.hour, self.minute = 12, 0
                        
                    self.show_add_task = True
                    
        imgui.end()
        
        # Logs window
        if self.show_logs_window:
            imgui.begin("Scheduler Logs", True)
            
            # Clear logs button
            if imgui.button("Clear Logs"):
                self.log_entries = []
                
            imgui.separator()
            
            # Display logs in scrollable region
            imgui.begin_child("LogsRegion", 0, 0, border=True)
            for log in self.log_entries:
                imgui.text(log)
            
            # Auto-scroll to bottom
            if imgui.get_scroll_y() >= imgui.get_scroll_max_y() - 20:
                imgui.set_scroll_here_y(1.0)
                
            imgui.end_child()
            imgui.end()
        
        # Add/Edit Task Dialog
        if self.show_add_task:
            imgui.begin("Add/Edit Task", True)
            
            # Task Name
            changed, name = imgui.input_text("Task Name", self.new_task.name, 100)
            if changed:
                self.new_task.name = name
                
            # Task Type
            current_type = self.new_task.task_type.value
            changed, task_type = imgui.combo("Task Type", current_type, ["Executable", "Script", "SSH Command"])
            if changed:
                self.new_task.task_type = TaskType(task_type)
                
            # Show SSH options if SSH Command is selected
            self.show_ssh_options = (self.new_task.task_type == TaskType.SSH_COMMAND)
                
            # Path/Command
            if self.new_task.task_type == TaskType.SSH_COMMAND:
                label = "SSH Command"
            else:
                label = "Path"
                
            changed, path = imgui.input_text(label, self.new_task.path, 200)
            if changed:
                self.new_task.path = path
                
            # Browse button for local files
            if self.new_task.task_type != TaskType.SSH_COMMAND:
                if imgui.button("Browse..."):
                    # In a real application, you would add file dialog here
                    # For simplicity, we'll just print a message
                    print("In a real application, this would open a file browser")
                    
            # SSH Options
            if self.show_ssh_options:
                imgui.separator()
                imgui.text("SSH Connection Settings:")
                
                changed, ssh_host = imgui.input_text("SSH Host", self.new_task.ssh_host, 100)
                if changed:
                    self.new_task.ssh_host = ssh_host
                    
                changed, ssh_port = imgui.input_int("SSH Port", self.new_task.ssh_port)
                if changed:
                    self.new_task.ssh_port = max(1, min(65535, ssh_port))
                    
                changed, ssh_username = imgui.input_text("SSH Username", self.new_task.ssh_username, 100)
                if changed:
                    self.new_task.ssh_username = ssh_username
                    
                # Password with hidden input
                changed, ssh_password = imgui.input_text(
                    "SSH Password", 
                    self.new_task.ssh_password, 
                    100, 
                    imgui.INPUT_TEXT_PASSWORD
                )
                if changed:
                    self.new_task.ssh_password = ssh_password
                    
                changed, ssh_key_path = imgui.input_text("SSH Key Path (optional)", self.new_task.ssh_key_path, 200)
                if changed:
                    self.new_task.ssh_key_path = ssh_key_path
                    
                if imgui.button("Browse SSH Key..."):
                    # In a real application, you would add file dialog here
                    print("In a real application, this would open a file browser for SSH key")
                    
            # Time selection
            imgui.separator()
            imgui.text("Schedule Settings:")
            
            changed_hour, self.hour = imgui.slider_int("Hour", self.hour, 0, 23)
            imgui.same_line()
            changed_min, self.minute = imgui.slider_int("Minute", self.minute, 0, 59)
            
            if changed_hour or changed_min:
                self.new_task.time = f"{self.hour:02d}:{self.minute:02d}"
                
            # Days of week
            imgui.text("Run on Days:")
            for i, day in enumerate(DAYS_OF_WEEK):
                changed, value = imgui.checkbox(day, self.new_task.days_of_week[i])
                if changed:
                    self.new_task.days_of_week[i] = value
                    
                # Place checkboxes in two columns
                if i % 2 == 0 and i < 6:
                    imgui.same_line()
                    
            # Date range
            imgui.text("Date Range (YYYY-MM-DD):")
            
            changed_start, date_buffer_start = imgui.input_text(
                "Start Date", "".join(self.date_buffer_start), 10, 
                imgui.INPUT_TEXT_CHARS_DECIMAL
            )
            
            if changed_start:
                self.date_buffer_start = list(date_buffer_start)
                if len(date_buffer_start) == 10:
                    self.new_task.start_date = date_buffer_start
                else:
                    self.new_task.start_date = ""
                    
            changed_end, date_buffer_end = imgui.input_text(
                "End Date", "".join(self.date_buffer_end), 10,
                imgui.INPUT_TEXT_CHARS_DECIMAL
            )
            
            if changed_end:
                self.date_buffer_end = list(date_buffer_end)
                if len(date_buffer_end) == 10:
                    self.new_task.end_date = date_buffer_end
                else:
                    self.new_task.end_date = ""
                    
            # Advanced options toggle
            if imgui.button("Advanced Options"):
                self.show_advanced_options = not self.show_advanced_options
                
            if self.show_advanced_options:
                imgui.separator()
                imgui.text("Advanced Options:")
                
                # Timeout
                changed, timeout = imgui.input_int("Timeout (seconds, 0=no timeout)", self.new_task.timeout)
                if changed:
                    self.new_task.timeout = max(0, timeout)
                    
                # Retry count
                changed, retry_count = imgui.input_int("Retry Count", self.new_task.retry_count)
                if changed:
                    self.new_task.retry_count = max(0, retry_count)
                    
                # Logging option
                changed, log_output = imgui.checkbox("Log Output", self.new_task.log_output)
                if changed:
                    self.new_task.log_output = log_output
                    # Notification option  
                changed, notify_on_failure = imgui.checkbox("Notify on Failure", self.new_task.notify_on_failure)
                if changed:
                    self.new_task.notify_on_failure = notify_on_failure
            
            imgui.separator()
            
            # Save button
            if imgui.button("Save Task"):
                # Validate task
                if not self.new_task.name:
                    self.add_log("Error: Task name cannot be empty")
                elif not self.new_task.path:
                    self.add_log("Error: Task path/command cannot be empty")
                elif not any(self.new_task.days_of_week):
                    self.add_log("Error: At least one day of week must be selected")
                elif self.new_task.task_type == TaskType.SSH_COMMAND and not self.new_task.ssh_host:
                    self.add_log("Error: SSH host cannot be empty for SSH commands")
                elif self.new_task.task_type == TaskType.SSH_COMMAND and not self.new_task.ssh_username:
                    self.add_log("Error: SSH username cannot be empty for SSH commands")
                elif self.new_task.task_type == TaskType.SSH_COMMAND and not (self.new_task.ssh_password or self.new_task.ssh_key_path):
                    self.add_log("Error: Either SSH password or key path must be provided")
                else:
                    # Format time properly
                    self.new_task.time = f"{self.hour:02d}:{self.minute:02d}"
                    
                    # Check if we're editing or adding
                    if self.selected_task >= 0 and self.selected_task < len(self.tasks):
                        # Update existing task
                        self.tasks[self.selected_task] = self.new_task
                        self.add_log(f"Updated task: {self.new_task.name}")
                    else:
                        # Add new task
                        self.tasks.append(self.new_task)
                        self.add_log(f"Added new task: {self.new_task.name}")
                        
                    self.save_config()
                    self.show_add_task = False
                    
            imgui.same_line()
            if imgui.button("Cancel"):
                self.show_add_task = False
                
            imgui.end()


def main():
    app = SchedulerApp()
    try:
        app.setup_gui()
    except Exception as e:
        print(f"Error in GUI: {e}")
    finally:
        app.task_runner_active = False
        # Give the task runner time to exit
        time.sleep(1)


if __name__ == "__main__":
    main()