#include "ImageService.h"
#include "HttpRequest.h"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <spawn.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <cerrno>
#include <algorithm>
#include <cctype>

extern char** environ;
namespace fs = std::filesystem;

namespace {
// Bound Python work across clients; each image still has its own output directory.
std::mutex processingMutex;

std::string readFile(fs::path const& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) throw HttpError{"404 Not Found", "Image file is unavailable"};
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

void runPython(fs::path const& input, fs::path const& output, bool previewOnly) {
    std::lock_guard<std::mutex> lock{processingMutex};
    std::vector<std::string> arguments{
        ".venv/bin/python", "python/image_analysis.py", "--input", input.string(),
        "--output-dir", output.string()
    };
    if (previewOnly) arguments.push_back("--preview-only");
    std::vector<char*> argv;
    for (auto& argument : arguments) argv.push_back(argument.data());
    argv.push_back(nullptr);
    pid_t pid;
    if (posix_spawn(&pid, argv[0], nullptr, nullptr, argv.data(), environ) != 0)
        throw std::runtime_error{"Python could not start. Install requirements.txt in .venv."};
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{90};
    int status = 0;
    while (true) {
        auto waited = waitpid(pid, &status, WNOHANG);
        if (waited == pid) break;
        if (waited < 0 && errno == EINTR) continue;
        if (waited < 0) throw std::runtime_error{"Could not wait for image processing"};
        if (std::chrono::steady_clock::now() >= deadline) {
            kill(pid, SIGKILL);
            while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
            throw std::runtime_error{"Image processing timed out"};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        auto error = output / "error.txt";
        throw std::runtime_error{fs::exists(error) ? readFile(error) : "Image processing failed; check Python dependencies and server output."};
    }
}

std::string imageJson(MedicalImage const& image) {
    return "{\"id\":" + std::to_string(image.id) +
        ",\"study_id\":" + std::to_string(image.study_id) +
        ",\"filename\":" + jsonString(image.filename) +
        ",\"uploaded_at\":" + jsonString(image.uploaded_at) +
        ",\"status\":" + jsonString(image.status) +
        ",\"result\":" + image.result + "}";
}

int positiveId(std::string const& text) {
    if (text.empty() || text.size() > 10 || !std::all_of(text.begin(), text.end(), [](unsigned char ch) { return std::isdigit(ch); }))
        throw HttpError{"400 Bad Request", "Invalid ID"};
    auto value = std::stoll(text);
    if (value < 1 || value > 2147483647) throw HttpError{"400 Bad Request", "Invalid ID"};
    return static_cast<int>(value);
}

void requireStudy(Database& db, int id) {
    for (auto const& study : db.getStudies()) if (study.get_id() == id) return;
    throw HttpError{"404 Not Found", "Study not found"};
}

std::string decodeFilename(std::string const& encoded) {
    std::string filename;
    for (std::size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%') {
            if (i + 2 >= encoded.size() || !std::isxdigit(static_cast<unsigned char>(encoded[i+1])) || !std::isxdigit(static_cast<unsigned char>(encoded[i+2])))
                throw HttpError{"400 Bad Request", "Invalid filename encoding"};
            filename += static_cast<char>(std::stoi(encoded.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else filename += encoded[i];
    }
    if (filename.empty() || filename.size() > 255 || filename == "." || filename == ".." ||
        std::any_of(filename.begin(), filename.end(), [](unsigned char ch) { return ch < 32 || ch == 127 || ch == '/' || ch == '\\'; }))
        throw HttpError{"400 Bad Request", "Use a filename without directory separators"};
    return filename;
}

MedicalImage storeImage(Database& db, int studyId, std::string const& filename, std::string const& bytes) {
    requireStudy(db, studyId);
    if (bytes.empty()) throw HttpError{"400 Bad Request", "Select a nonempty image"};
    fs::create_directories("data/images");
    std::string pattern = "data/images/upload-XXXXXX";
    if (!mkdtemp(pattern.data())) throw std::runtime_error{"Could not create image storage"};
    fs::path directory{pattern};
    try {
        auto source = directory / "source";
        std::ofstream file{source, std::ios::binary};
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        file.close();
        if (!file) throw std::runtime_error{"Could not save upload"};
        runPython(source, directory, true);
        int id = db.addImage(studyId, filename, source.string());
        return db.getImage(id);
    } catch (...) {
        fs::remove_all(directory);
        throw;
    }
}
}

bool handleImageRequest(std::string const& method, std::string const& path,
                        std::map<std::string, std::string> const& headers,
                        std::string const& body, Database& db, std::string& response) {
    if (method == "GET" && path == "/sample-image") {
        response = httpResponse("200 OK", readFile("ct_image.png"), "image/png");
        return true;
    }
    if (path.rfind("/studies/", 0) == 0) {
        auto slash = path.find('/', 9);
        if (slash == std::string::npos) return false;
        auto action = path.substr(slash);
        if (action != "/images" && action != "/sample-image") return false;
        int studyId = positiveId(path.substr(9, slash - 9));
        requireStudy(db, studyId);
        if (method == "GET" && action == "/images") {
            std::string result = "[";
            for (auto const& image : db.getImages(studyId)) {
                if (result.size() > 1) result += ',';
                result += imageJson(image);
            }
            response = httpResponse("200 OK", result + ']');
        } else if (method == "POST") {
            auto filename = headers.find("x-filename");
            if (action == "/images" && filename == headers.end())
                throw HttpError{"400 Bad Request", "X-Filename is required"};
            auto image = storeImage(db, studyId,
                action == "/images" ? decodeFilename(filename->second) : "ct_image.png",
                action == "/images" ? body : readFile("ct_image.png"));
            response = httpResponse("201 Created", imageJson(image));
        } else throw HttpError{"405 Method Not Allowed", "Method not allowed"};
        return true;
    }
    if (path.rfind("/images/", 0) != 0) return false;
    auto slash = path.find('/', 8);
    int id = positiveId(path.substr(8, slash == std::string::npos ? slash : slash - 8));
    MedicalImage image;
    try { image = db.getImage(id); }
    catch (std::out_of_range const&) { throw HttpError{"404 Not Found", "Image not found"}; }
    auto action = slash == std::string::npos ? "" : path.substr(slash);
    auto directory = fs::path{image.storage_path}.parent_path();
    if (method == "GET" && action.empty()) {
        response = httpResponse("200 OK", imageJson(image));
    } else if (method == "GET" && (action == "/preview" || action == "/segmentation")) {
        if (action == "/segmentation" && image.status != "complete")
            throw HttpError{"409 Conflict", "Run analysis before viewing segmentation"};
        response = httpResponse("200 OK", readFile(directory / (action == "/preview" ? "ct_image.png" : "clustered_ct.png")), "image/png");
    } else if (method == "POST" && action == "/analysis") {
        if (!db.startImageAnalysis(id)) throw HttpError{"409 Conflict", "Analysis is already running"};
        try {
            runPython(image.storage_path, directory, false);
            db.finishImageAnalysis(id, "complete", readFile(directory / "analysis_result.txt"));
        } catch (std::exception const& error) {
            db.finishImageAnalysis(id, "failed", "{\"error\":" + jsonString(error.what()) + "}");
        }
        response = httpResponse("200 OK", imageJson(db.getImage(id)));
    } else throw HttpError{"404 Not Found", "Image endpoint not found"};
    return true;
}
