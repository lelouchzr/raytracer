#pragma once
#include "rt/studio/StudioDocument.hpp"
#include "rt/studio/StudioRenderer.hpp"
#include <cstdint>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace rt {
enum class StudioSelection { Object, Light, Material, Camera, Environment, Render };
enum class StudioFileAction { None, Open, Save, Texture, Normal, Environment, Mesh, Png, Hdr };

class StudioApp {
    StudioDocument document_;
    StudioRenderer rendering_;
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    SDL_Texture *texture_ = nullptr;
    bool platformReady_ = false, rendererReady_ = false;
    bool live_ = true, dirty_ = false, jsonOpen_ = false, overwrite_ = false;
    StudioSelection selection_ = StudioSelection::Material;
    int selected_ = 0;
    std::string material_ = "gold", message_, jsonText_, jsonError_;
    StudioFileAction fileAction_ = StudioFileAction::None;
    std::filesystem::path folder_;
    std::string filename_;
    std::vector<std::string> undo_, redo_;
    int imageWidth_ = 0, imageHeight_ = 0, displayedRows_ = -1;
    unsigned long long displayedGeneration_ = 0;
    int displayedPass_ = -1;
    unsigned propertyEdit_ = 0;
    std::vector<std::uint8_t> rgba_;
    std::chrono::steady_clock::time_point lastRequest_;
    void shutdown() noexcept;
    void header(float width);
    void tree(float height);
    void inspector(float width, float height);
    void viewport(float width, float height);
    void dialogs();
    void fileDialog();
    void chooseFile(StudioFileAction, const std::string &);
    void applyFile();
    void updateImage();
    void edit();
    void remember();
    void undo(bool forward);
    Json *selectedNode();
    void objectProperties(Json &);
    void materialProperties(Json &);
    void lightProperties(Json &);
    void cameraProperties(Json &);
    void renderProperties(Json &);
    void environmentProperties();
    void addObject(const std::string &);
    void addLight(const std::string &);
    void addMaterial();
    void duplicate();
    void remove();
    void savePng(const std::filesystem::path &, bool wholeWindow = false);

  public:
    explicit StudioApp(const std::filesystem::path &);
    ~StudioApp();
    StudioApp(const StudioApp &) = delete;
    StudioApp &operator=(const StudioApp &) = delete;
    void run(int frameLimit, const std::filesystem::path &screenshot);
};
} // namespace rt
