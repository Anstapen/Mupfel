function download_progress(total, current)
    local ratio = current / total;
    ratio = math.min(math.max(ratio, 0), 1);
    local percent = math.floor(ratio * 100);
    print("Download progress (" .. percent .. "%/100%)")
end

function check_nlohmann()
    print("Checking for nlohmann json...")
    print(os.getcwd())
    os.chdir("Vendor")
    if(os.isdir("Sources") == false) then
        os.mkdir("Sources")
    end
    os.chdir("Sources")
    if(os.isdir("nlohmann") == false) then
        os.mkdir("nlohmann")
    end
    os.chdir("nlohmann")
    if(not os.isfile("json.hpp")) then
        print("nlohmann json not found, downloading from https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp")
        local result_str, response_code = http.download("https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp", "json.hpp", {
            progress = download_progress,
            headers = { "From: Premake", "Referer: Premake" }
        })
    end
    os.chdir("../../../")
end

function check_vulkan_starter()
    print("Checking for vulkan_starter (Ping)...")
    os.chdir("Vendor")
    if(os.isdir("Sources") == false) then
        os.mkdir("Sources")
    end
    os.chdir("Sources")
    if(os.isdir("vulkan_starter-main") == false) then
        if(not os.isfile("vulkan_starter-main.zip")) then
            print("vulkan_starter not found, downloading from https://github.com/Anstapen/vulkan_starter/archive/refs/heads/main.zip")
            local result_str, response_code = http.download("https://github.com/Anstapen/vulkan_starter/archive/refs/heads/main.zip", "vulkan_starter-main.zip", {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping to " ..  os.getcwd())
        zip.extract("vulkan_starter-main.zip", os.getcwd())
        os.remove("vulkan_starter-main.zip")
    end
    os.chdir("../../")
end

function check_glfw()
    print("Checking for glfw...")
    os.chdir("Vendor")
    if(os.isdir("Sources") == false) then
        os.mkdir("Sources")
    end
    os.chdir("Sources")
    if(os.isdir("glfw-3.4.bin.WIN64") == false) then
        if(not os.isfile("glfw.zip")) then
            print("glfw not found, downloading from https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip")
            local result_str, response_code = http.download("https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip", "glfw.zip", {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping to " ..  os.getcwd())
        zip.extract("glfw.zip", os.getcwd())
        os.remove("glfw.zip")
    end
    os.chdir("../../")
end

function check_spdlog()
    print("Checking for spdlog...")
    os.chdir("Vendor")
    if(os.isdir("Sources") == false) then
        os.mkdir("Sources")
    end
    os.chdir("Sources")
    if(os.isdir("spdlog-1.17.0") == false) then
        if(not os.isfile("spdlog.zip")) then
            print("spdlog not found, downloading from https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.zip")
            local result_str, response_code = http.download("https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.zip", "spdlog.zip", {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping to " ..  os.getcwd())
        zip.extract("spdlog.zip", os.getcwd())
        os.remove("spdlog.zip")
    end
    os.chdir("../../")
end

function check_stb_image()
    print("Checking for stb_image...")
    os.chdir("Vendor")
    if(os.isdir("Sources") == false) then
        os.mkdir("Sources")
    end
    os.chdir("Sources")
    if(os.isdir("stb") == false) then
        os.mkdir("stb")
    end
    if(not os.isfile("stb/stb_image.h")) then
        print("stb_image.h not found, downloading from https://raw.githubusercontent.com/nothings/stb/master/stb_image.h")
        local result_str, response_code = http.download("https://raw.githubusercontent.com/nothings/stb/master/stb_image.h", "stb/stb_image.h", {
            progress = download_progress,
            headers = { "From: Premake", "Referer: Premake" }
        })
    end
    os.chdir("../../")
end

function check_imgui()
    print("Checking for imgui...")
    os.chdir("Vendor")
    if(os.isdir("Sources") == false) then
        os.mkdir("Sources")
    end
    os.chdir("Sources")
    if(os.isdir("imgui-6029ee3789a2b7898f6423ec0c88cc4e5425f5a9") == false) then
        if(not os.isfile("imgui.zip")) then
            print("imgui not found, downloading from https://github.com/ocornut/imgui/archive/6029ee3789a2b7898f6423ec0c88cc4e5425f5a9.zip")
            local result_str, response_code = http.download("https://github.com/ocornut/imgui/archive/6029ee3789a2b7898f6423ec0c88cc4e5425f5a9.zip", "imgui.zip", {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping to " ..  os.getcwd())
        zip.extract("imgui.zip", os.getcwd())
        os.remove("imgui.zip")
    end
    os.chdir("../../")
end

function build_externals()
     print("Checking external dependencies...")
     check_nlohmann()
     check_vulkan_starter()
     check_spdlog()
     check_stb_image()
     check_imgui()
     filter "system:windows"
        check_glfw()
end

build_externals()