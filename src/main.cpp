#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdlib>     // For system() - use with caution!
#include <sstream>
#include <fstream>     // For file operations
#include <thread>      // For sleep and thread management
#include <chrono>      // For time handling
#include <iomanip>     // For output formatting

#include "lib/include/lib.h"

// --- Service Definition ---
// Structure to define a manageable service (likely a Docker container)
struct ServiceDefinition {
    std::string id;
    std::string name;
    std::string description;
    std::string dockerImage;
    std::vector<std::string> ports;
    std::vector<std::string> volumes; 
    std::map<std::string, std::string> environment;
    std::string command;
    bool isLLM = false;
};

// --- Placeholder for actual service definitions ---
// In a real application, this would likely be loaded from a configuration file (JSON, YAML)
std::vector<ServiceDefinition> predefinedServices = {
    {
        "web_server_1", "Simple Web Server", "An Nginx web server",
        "nginx:latest", {"8080:80"}, {"/data/web:/usr/share/nginx/html:ro"}, {}, "", false
    },
    {
        "data_processor", "Data Processing Task", "A custom data processing container",
        "my_processor_image:v1.2", {}, {}, {{"API_KEY", "dummy_key"}, {"INPUT_DIR", "/data"}}, "/app/run_processor.sh", false
    },
    {
        "llm_model_a", "LLM Model A (Ollama)", "Runs a specific LLM using Ollama",
        "ollama/ollama", {"11434:11434"}, {"ollama_data:/root/.ollama"}, {}, "", true // Assuming Ollama runs as default entrypoint
    },
    {
        "mongodb", "MongoDB Database", "MongoDB NoSQL database",
        "mongo:latest", {"27017:27017"}, {"mongo_data:/data/db"}, {{"MONGO_INITDB_ROOT_USERNAME", "admin"}, {"MONGO_INITDB_ROOT_PASSWORD", "password"}}, "", false
    },
    {
        "redis_cache", "Redis Cache", "Redis in-memory data structure store",
        "redis:latest", {"6379:6379"}, {"redis_data:/data"}, {}, "", false
    }
};

// --- Helper Functions ---

// Helper function to execute a command and return its output
std::string executeCommandWithOutput(const std::string& command) {
    std::string output;
    char buffer[128];
    FILE* pipe = popen(command.c_str(), "r");
    
    if (!pipe) {
        return "Error executing command.";
    }
    
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != nullptr) {
            output += buffer;
        }
    }
    
    pclose(pipe);
    return output;
}

// Finds a service definition by its ID
const ServiceDefinition* findServiceById(const std::string& id) {
    for (const auto& service : predefinedServices) {
        if (service.id == id) {
            return &service;
        }
    }
    return nullptr;
}

// **WARNING:** Using system() is generally insecure and not robust.
// A real application should use the Docker Engine API via a C++ client library
// or inter-process communication (IPC) with the Docker daemon.
int runDockerCommand(const std::string& command) {
    std::cout << "Executing Docker Command: " << command << std::endl;
    // In a real service, redirect stdout/stderr appropriately
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Docker command failed with exit code: " << result << std::endl;
    }
    return result;
}

// Constructs and runs a 'docker run' command for a service
bool startService(const ServiceDefinition& service) {
    std::stringstream dockerCmd;
    dockerCmd << "docker run -d --name " << service.id; // Run detached, assign name

    for (const auto& port : service.ports) {
        dockerCmd << " -p " << port;
    }
    for (const auto& volume : service.volumes) {
        dockerCmd << " -v " << volume;
    }
    for (const auto& env : service.environment) {
        dockerCmd << " -e " << env.first << "=" << env.second;
    }
    // Add other options here (network, restart policy etc.)

    dockerCmd << " " << service.dockerImage;

    if (!service.command.empty()) {
        dockerCmd << " " << service.command;
    }

    return runDockerCommand(dockerCmd.str()) == 0;
}

// Constructs and runs a 'docker stop' and 'docker rm' command
bool stopService(const std::string& serviceId) {
    std::string stopCmd = "docker stop " + serviceId;
    std::string rmCmd = "docker rm " + serviceId;
    bool stopped = runDockerCommand(stopCmd) == 0;
    // Attempt removal even if stop failed (might already be stopped)
    bool removed = runDockerCommand(rmCmd) == 0;
    return stopped || removed; // Consider success if removed
}

// --- Docker Container Management Functions ---

// List all running Docker containers
void listRunningContainers() {
    std::cout << "\n=== Running Docker Containers ===\n" << std::endl;
    runDockerCommand("docker ps --format \"table {{.ID}}\t{{.Names}}\t{{.Image}}\t{{.Status}}\t{{.Ports}}\"");
    std::cout << std::endl;
}

// List all Docker containers (running and stopped)
void listAllContainers() {
    std::cout << "\n=== All Docker Containers ===\n" << std::endl;
    runDockerCommand("docker ps -a --format \"table {{.ID}}\t{{.Names}}\t{{.Image}}\t{{.Status}}\"");
    std::cout << std::endl;
}

// List available Docker images
void listDockerImages() {
    std::cout << "\n=== Available Docker Images ===\n" << std::endl;
    runDockerCommand("docker images --format \"table {{.Repository}}:{{.Tag}}\t{{.ID}}\t{{.Size}}\"");
    std::cout << std::endl;
}

// Pull a Docker image
bool pullDockerImage(const std::string& image) {
    std::cout << "Pulling Docker image: " << image << std::endl;
    return runDockerCommand("docker pull " + image) == 0;
}

// --- System Resource Monitoring Functions ---

// Check available disk space
void checkDiskSpace() {
    std::cout << "\n=== Disk Space Information ===\n" << std::endl;
    
    #ifdef _WIN32
        runDockerCommand("wmic logicaldisk get deviceid,freespace,size");
    #else
        runDockerCommand("df -h");
    #endif
    
    std::cout << std::endl;
}

// Check memory usage
void checkMemoryUsage() {
    std::cout << "\n=== Memory Usage Information ===\n" << std::endl;
    
    #ifdef _WIN32
        runDockerCommand("wmic OS get FreePhysicalMemory,TotalVisibleMemorySize");
    #else
        runDockerCommand("free -h");
    #endif
    
    std::cout << std::endl;
}

// Check CPU usage
void checkCpuUsage() {
    std::cout << "\n=== CPU Usage Information ===\n" << std::endl;
    
    #ifdef _WIN32
        runDockerCommand("wmic cpu get LoadPercentage");
    #else
        runDockerCommand("top -bn1 | grep \"Cpu(s)\"");
    #endif
    
    std::cout << std::endl;
}

// Check Docker resource usage
void checkDockerResourceUsage() {
    std::cout << "\n=== Docker Resource Usage ===\n" << std::endl;
    runDockerCommand("docker stats --no-stream --format \"table {{.Name}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.NetIO}}\t{{.BlockIO}}\"");
    std::cout << std::endl;
}

// Get system information summary
void getSystemInfo() {
    std::cout << "\n====== System Information Summary ======\n" << std::endl;
    checkDiskSpace();
    checkMemoryUsage();
    checkCpuUsage();
    checkDockerResourceUsage();
}

// --- System Maintenance Functions ---

// Clean up Docker system (remove unused containers, images, etc.)
void cleanupDockerSystem() {
    std::cout << "\n=== Cleaning Up Docker System ===\n" << std::endl;
    
    std::cout << "Removing stopped containers..." << std::endl;
    runDockerCommand("docker container prune -f");
    
    std::cout << "Removing unused images..." << std::endl;
    runDockerCommand("docker image prune -f");
    
    std::cout << "Removing unused volumes..." << std::endl;
    runDockerCommand("docker volume prune -f");
    
    std::cout << "Removing unused networks..." << std::endl;
    runDockerCommand("docker network prune -f");
    
    std::cout << "Docker system cleanup complete." << std::endl;
}

// Clear system cache (platform-specific)
void clearSystemCache() {
    std::cout << "\n=== Clearing System Cache ===\n" << std::endl;
    
    #ifdef _WIN32
        std::cout << "Clearing Windows temporary files..." << std::endl;
        runDockerCommand("del /q /s %temp%\\*");
    #else
        std::cout << "Dropping system caches..." << std::endl;
        std::cout << "Note: This requires sudo/root permissions." << std::endl;
        runDockerCommand("sudo sh -c \"sync; echo 3 > /proc/sys/vm/drop_caches\"");
    #endif
    
    std::cout << "System cache clearing complete." << std::endl;
}

// Schedule regular maintenance
void scheduleRegularMaintenance() {
    std::cout << "\n=== Scheduling Regular Maintenance ===\n" << std::endl;
    
    std::cout << "This feature would typically use cron (Linux) or Task Scheduler (Windows)." << std::endl;
    std::cout << "For demonstration, we'll just run a cleanup now." << std::endl;
    
    cleanupDockerSystem();
    
    std::cout << "To schedule regular maintenance, you would typically:" << std::endl;
    std::cout << "1. Create a script that runs the cleanup commands" << std::endl;
    std::cout << "2. Add it to cron (Linux) or Task Scheduler (Windows)" << std::endl;
    std::cout << "3. Set it to run at regular intervals (e.g., daily, weekly)" << std::endl;
}

// --- User Interface Functions ---

// Display menu of available services
void displayServiceMenu() {
    std::cout << "\n=== Available Services ===\n" << std::endl;
    std::cout << std::setw(5) << "No." << std::setw(20) << "ID" << std::setw(30) << "Name" << "Description" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    for (size_t i = 0; i < predefinedServices.size(); ++i) {
        std::cout << std::setw(5) << (i + 1) << std::setw(20) << predefinedServices[i].id 
                  << std::setw(30) << predefinedServices[i].name 
                  << predefinedServices[i].description << std::endl;
    }
    std::cout << std::endl;
}

// Display main menu
void displayMainMenu() {
    std::cout << "\n======== Docker Service Manager ========\n" << std::endl;
    std::cout << "1. List running Docker containers" << std::endl;
    std::cout << "2. List all Docker containers" << std::endl;
    std::cout << "3. List available Docker images" << std::endl;
    std::cout << "4. Start a service" << std::endl;
    std::cout << "5. Stop a service" << std::endl;
    std::cout << "6. Pull a Docker image" << std::endl;
    std::cout << "7. Check system resources" << std::endl;
    std::cout << "8. Perform system maintenance" << std::endl;
    std::cout << "9. Chat Service" << std::endl;
    std::cout << "10. Exit" << std::endl;
    std::cout << "\nEnter your choice (1-10): ";
}

// Display maintenance menu
void displayMaintenanceMenu() {
    std::cout << "\n======== System Maintenance ========\n" << std::endl;
    std::cout << "1. Clean up Docker system" << std::endl;
    std::cout << "2. Clear system cache" << std::endl;
    std::cout << "3. Schedule regular maintenance" << std::endl;
    std::cout << "4. Return to main menu" << std::endl;
    std::cout << "\nEnter your choice (1-4): ";
}

// User choice to start a service
void userStartService() {
    displayServiceMenu();
    
    int serviceIndex;
    std::cout << "Enter the service number to start (1-" << predefinedServices.size() << "): ";
    std::cin >> serviceIndex;
    
    if (serviceIndex < 1 || serviceIndex > static_cast<int>(predefinedServices.size())) {
        std::cerr << "Invalid selection. Please try again." << std::endl;
        return;
    }
    
    const ServiceDefinition& service = predefinedServices[serviceIndex - 1];
    std::cout << "Starting service: " << service.name << " (" << service.id << ")" << std::endl;
    
    if (startService(service)) {
        std::cout << "Service started successfully." << std::endl;
    } else {
        std::cerr << "Failed to start service." << std::endl;
    }
}

// User choice to stop a service
void userStopService() {
    std::string serviceId;
    
    // First, show running containers so user can see what's available to stop
    listRunningContainers();
    
    std::cout << "Enter the service ID to stop: ";
    std::cin >> serviceId;
    
    std::cout << "Stopping service: " << serviceId << std::endl;
    
    if (stopService(serviceId)) {
        std::cout << "Service stopped successfully." << std::endl;
    } else {
        std::cerr << "Failed to stop service." << std::endl;
    }
}

// User choice to pull a Docker image
void userPullImage() {
    std::string imageName;
    std::cin.ignore(); // Clear any previous input
    
    std::cout << "Enter the Docker image to pull (e.g., nginx:latest): ";
    std::getline(std::cin, imageName);
    
    if (pullDockerImage(imageName)) {
        std::cout << "Image pulled successfully." << std::endl;
    } else {
        std::cerr << "Failed to pull image." << std::endl;
    }
}

// Handle maintenance menu
void handleMaintenanceMenu() {
    int choice;
    bool exit = false;
    
    while (!exit) {
        displayMaintenanceMenu();
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                cleanupDockerSystem();
                break;
            case 2:
                clearSystemCache();
                break;
            case 3:
                scheduleRegularMaintenance();
                break;
            case 4:
                exit = true;
                break;
            default:
                std::cerr << "Invalid choice. Please try again." << std::endl;
        }
    }
}

// Function to integrate chat service with the console manager
void add_chat_service_to_menu() {
    ChatService::display_menu();
    ChatService::handle_menu_choice();
}

// --- Main Application Logic ---

int main(int argc, char *argv[]) {
    std::cout << "Starting Docker Service Manager..." << std::endl;
    
    // Initialize any required services
    // initialize_buffers();
    
    int choice;
    bool exit = false;
    
    while (!exit) {
        displayMainMenu();
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                listRunningContainers();
                break;
            case 2:
                listAllContainers();
                break;
            case 3:
                listDockerImages();
                break;
            case 4:
                userStartService();
                break;
            case 5:
                userStopService();
                break;
            case 6:
                userPullImage();
                break;
            case 7:
                getSystemInfo();
                break;
            case 8:
                handleMaintenanceMenu();
                break;
            case 9:
                add_chat_service_to_menu();
                break;
            case 10:
                std::cout << "Exiting Docker Service Manager. Goodbye!" << std::endl;
                exit = true;
                break;
            default:
                std::cerr << "Invalid choice. Please try again." << std::endl;
        }
    }
    
    return 0;
}