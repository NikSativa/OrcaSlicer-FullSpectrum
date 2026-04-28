#ifndef slic3r_Format_OBJ_hpp_
#define slic3r_Format_OBJ_hpp_
#include "libslic3r/Color.hpp"
#include "libslic3r/Format/objparser.hpp"
#include <unordered_map>
namespace Slic3r {

class TriangleMesh;
class Model;
class ModelObject;
typedef std::function<void(std::vector<RGBA> &input_colors, bool is_single_color, std::vector<unsigned char> &filament_ids, unsigned char &first_extruder_id)> ObjImportColorFn;
// Load an OBJ file into a provided model.
struct ObjInfo {
    std::vector<RGBA> vertex_colors;
    std::vector<RGBA> face_colors;
    // SnapOrka: ported from BambuStudio v2.6.0 — preserve full multi-material data when importing OBJ+.mtl
    std::vector<RGBA>                 mtl_colors;
    std::vector<std::string>          mtl_color_names;
    std::vector<ObjParser::ObjUseMtl> usemtls; // for origin render
    bool                              first_time_using_makerlab{false};
    bool                              is_single_mtl{false};
    std::string                       lost_material_name;
    std::vector<std::array<Vec2f,3>> uvs;
    std::string        obj_dircetory;
    std::map<std::string,bool>  pngs;
    std::unordered_map<int, std::string> uv_map_pngs;
    bool              has_uv_png{false};

    // SnapOrka: ported from BambuStudio — MakerLab metadata (region/name/id) embedded in OBJ comments
    std::string ml_region;
    std::string ml_name;
    std::string ml_id;
};
extern bool load_obj(const char *path, TriangleMesh *mesh, ObjInfo &vertex_colors, std::string &message);
extern bool load_obj(const char *path, Model *model, ObjInfo &vertex_colors, std::string &message, const char *object_name = nullptr);

extern bool store_obj(const char *path, TriangleMesh *mesh);
extern bool store_obj(const char *path, ModelObject *model);
extern bool store_obj(const char *path, Model *model);

}; // namespace Slic3r

#endif /* slic3r_Format_OBJ_hpp_ */
