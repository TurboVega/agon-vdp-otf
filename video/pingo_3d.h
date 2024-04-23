#ifndef PINGO_3D_H
#define PINGO_3D_H

#include <stdint.h>
#include <string.h>
#include <agon.h>
#include <map>
#include "esp_heap_caps.h"
#include "sprites.h"

namespace p3d {

    extern "C" {

        #include "pingo/assets/teapot.h"

        #include "pingo/render/mesh.h"
        #include "pingo/render/object.h"
        #include "pingo/render/pixel.h"
        #include "pingo/render/renderer.h"
        #include "pingo/render/scene.h"
        #include "pingo/render/backend.h"
        #include "pingo/render/depth.h"

    } // extern "C"

} // namespace p3d

#define PINGO_3D_CONTROL_TAG    0x43443350 // "P3DC"

class VDUStreamProcessor;

typedef struct tag_TexObject {
    p3d::Texture    m_texture;
    p3d::Material   m_material;
    p3d::Object     m_object;
    p3d::Vec3f      m_scale;
    p3d::Vec3f      m_rotation;
    p3d::Vec3f      m_translation;
} TexObject;

typedef struct tag_Pingo3dControl {
    uint32_t            m_tag;              // Used to verify the existence of this structure
    uint32_t            m_size;             // Used to verify the existence of this structure
    VDUStreamProcessor* m_proc;             // Used by subcommands to obtain more data
    p3d::BackEnd        m_backend;          // Used by the renderer
    p3d::Pixel*         m_frame;            // Frame buffer for rendered pixels
    p3d::PingoDepth*    m_zeta;             // Zeta buffer for depth information
    uint16_t            m_width;            // Width of final render in pixels
    uint16_t            m_height;           // Height of final render in pixels
    std::map<uint16_t, p3d::Mesh>* m_meshes;    // Map of meshes for use by objects
    std::map<uint16_t, TexObject>* m_objects;   // Map of textured objects that use meshes and have transforms

    // VDU 23, 0, &A0, sid; &48, 0, 1 :  Initialize Control Structure
    void initialize(VDUStreamProcessor& processor, uint16_t width, uint16_t height) {
        memset(this, 0, sizeof(tag_Pingo3dControl));
        m_tag = PINGO_3D_CONTROL_TAG;
        m_size = sizeof(tag_Pingo3dControl);

        auto frame_size = (uint32_t) width * (uint32_t) height;
        m_frame = (p3d::Pixel*) heap_caps_malloc(sizeof(p3d::Pixel) * frame_size,
            MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        m_zeta = (p3d::PingoDepth*) heap_caps_malloc(sizeof(p3d::PingoDepth) * frame_size,
            MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);

        m_backend.init = &static_init;
        m_backend.beforeRender = &static_before_render;
        m_backend.afterRender = &static_after_render;
        m_backend.getFrameBuffer = &static_get_frame_buffer;
        m_backend.getZetaBuffer = &static_get_zeta_buffer;
        m_backend.drawPixel = 0;
        m_backend.clientCustomData = (void*) this;

        m_meshes = new std::map<uint16_t, p3d::Mesh>;
        m_objects = new std::map<uint16_t, TexObject>;
    }

    static void static_init(p3d::Renderer* ren, p3d::BackEnd* backEnd, p3d::Vec4i _rect) {
        //rect = _rect;
    }

    static void static_before_render(p3d::Renderer* ren, p3d::BackEnd* backEnd) {
    }

    static void static_after_render(p3d::Renderer* ren, p3d::BackEnd* backEnd) {
    }

    static p3d::Pixel* static_get_frame_buffer(p3d::Renderer* ren, p3d::BackEnd* backEnd) {
        auto p_this = (struct tag_Pingo3dControl*) backEnd->clientCustomData;
        return p_this->m_frame;
    }

    static p3d::PingoDepth* static_get_zeta_buffer(p3d::Renderer* ren, p3d::BackEnd* backEnd) {
        auto p_this = (struct tag_Pingo3dControl*) backEnd->clientCustomData;
        return p_this->m_zeta;
    }

    // VDU 23, 0, &A0, sid; &48, 0, 0 :  Deinitialize Control Structure
    void deinitialize(VDUStreamProcessor& processor) {
    }

    bool validate() {
        return (m_tag == PINGO_3D_CONTROL_TAG &&
                m_size == sizeof(tag_Pingo3dControl));
    }

    void handle_subcommand(VDUStreamProcessor& processor, uint8_t subcmd) {
        debug_log("P3D: handle_subcommand(%hu)\n", subcmd);
        m_proc = &processor;
        switch (subcmd) {
            case 1: define_mesh_vertices(); break;
            case 2: set_mesh_vertex_indexes(); break;
            case 3: define_texture_coordinates(); break;
            case 4: set_texture_coordinate_indexes(); break;
            case 5: create_object(); break;
            case 6: set_object_x_scale_factor(); break;
            case 7: set_object_y_scale_factor(); break;
            case 8: set_object_z_scale_factor(); break;
            case 9: set_object_xyz_scale_factors(); break;
            case 10: set_object_x_rotation_angle(); break;
            case 11: set_object_y_rotation_angle(); break;
            case 12: set_object_z_rotation_angle(); break;
            case 13: set_object_xyz_rotation_angles(); break;
            case 14: set_object_x_translation_distance(); break;
            case 15: set_object_y_translation_distance(); break;
            case 16: set_object_z_translation_distance(); break;
            case 17: set_object_xyz_translation_distances(); break;
            case 18: render_to_bitmap(); break;
        }
    }

    p3d::Mesh* establish_mesh(uint16_t mid) {
        auto mesh_iter = m_meshes->find(mid);
        if (mesh_iter == m_meshes->end()) {
            p3d::Mesh mesh;
            memset(&mesh, 0, sizeof(mesh));
            (*m_meshes).insert(std::pair<uint16_t, p3d::Mesh>(mid, mesh));
            return &m_meshes->find(mid)->second;
        } else {
            return &mesh_iter->second;
        }
    }

    p3d::Mesh* get_mesh() {
        auto mid = m_proc->readWord_t();
        if (mid >= 0) {
            return establish_mesh(mid);
        }
        return NULL;
    }

    TexObject* establish_object(uint16_t mid) {
        auto object_iter = m_objects->find(mid);
        if (object_iter == m_objects->end()) {
            TexObject object;
            memset(&object, 0, sizeof(object));
            object.m_object.material = &object.m_material;
            object.m_material.texture = &object.m_texture;
            object.m_scale = p3d::Vec3f { 1.0f, 1.0f, 1.0f };
            (*m_objects).insert(std::pair<uint16_t, TexObject>(mid, object));
            return &m_objects->find(mid)->second;
        } else {
            return &object_iter->second;
        }
    }

    TexObject* get_object() {
        auto mid = m_proc->readWord_t();
        if (mid >= 0) {
            return establish_object(mid);
        }
        return NULL;
    }

    // VDU 23, 0, &A0, sid; &48, 1, mid; n; x0; y0; z0; ... :  Define Mesh Vertices
    void define_mesh_vertices() {
        auto mesh = get_mesh();
        if (mesh->positions) {
            heap_caps_free(mesh->positions);
            mesh->positions = NULL;
        }
        auto n = (uint32_t) m_proc->readWord_t();
        if (n > 0) {
            mesh->positions = (p3d::Vec3f*) heap_caps_malloc(n*sizeof(p3d::Vec3f), MALLOC_CAP_SPIRAM);
            auto pos = mesh->positions;
            for (uint32_t i = 0; i < n; i++) {
                uint16_t x = m_proc->readWord_t();
                uint16_t y = m_proc->readWord_t();
                uint16_t z = m_proc->readWord_t();
                if (pos) {
                    pos->x = x;
                    pos->y = y;
                    pos->z = z;
                    pos++;
                }
            }
        }
    }

    // VDU 23, 0, &A0, sid; &48, 2, mid; n; i0; ... :  Set Mesh Vertex Indexes
    void set_mesh_vertex_indexes() {
        auto mesh = get_mesh();
        if (mesh->pos_indices) {
            heap_caps_free(mesh->pos_indices);
            mesh->pos_indices = NULL;
            mesh->indexes_count = 0;
        }
        auto n = (uint32_t) m_proc->readWord_t();
        if (n > 0) {
            mesh->indexes_count = n;
            mesh->pos_indices = (uint16_t*) heap_caps_malloc(n*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
            auto idx = mesh->pos_indices;
            for (uint32_t i = 0; i < n; i++) {
                uint16_t index = m_proc->readWord_t();
                if (idx) {
                    *idx++ = index;
                }
            }
        }
    }

    // VDU 23, 0, &A0, sid; &48, 3, mid; n; u0; v0; ... :  Define Texture Coordinates
    void define_texture_coordinates() {
        auto mesh = get_mesh();
        if (mesh->textCoord) {
            heap_caps_free(mesh->textCoord);
            mesh->textCoord = NULL;
        }
        auto n = (uint32_t) m_proc->readWord_t();
        if (n > 0) {
            mesh->textCoord = (p3d::Vec2f*) heap_caps_malloc(n*sizeof(p3d::Vec2f), MALLOC_CAP_SPIRAM);
            auto coord = mesh->textCoord;
            for (uint32_t i = 0; i < n; i++) {
                uint16_t u = m_proc->readWord_t();
                uint16_t v = m_proc->readWord_t();
                if (coord) {
                    coord->x = u;
                    coord->y = v;
                    coord++;
                }
            }
        }
    }

    // VDU 23, 0, &A0, sid; &48, 4, mid; n; i0; ... :  Set Texture Coordinate Indexes
    void set_texture_coordinate_indexes() {
        auto mesh = get_mesh();
        if (mesh->tex_indices) {
            heap_caps_free(mesh->tex_indices);
            mesh->tex_indices = NULL;
        }
        auto n = (uint32_t) m_proc->readWord_t();
        if (n > 0) {
            mesh->tex_indices = (uint16_t*) heap_caps_malloc(n*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
            auto idx = mesh->tex_indices;
            for (uint32_t i = 0; i < n; i++) {
                uint16_t index = m_proc->readWord_t();
                if (idx && (i < mesh->indexes_count)) {
                    *idx++ = index;
                }
            }
        }
    }

    // VDU 23, 0, &A0, sid; &48, 5, oid; mid; bmid; :  Create Object
    void create_object() {
        auto object = get_object();
        auto mesh = get_mesh();
        auto bmid = m_proc->readWord_t();
        if (object && mesh && bmid) {
            auto stored_bitmap = getBitmap(bmid);
            if (stored_bitmap) {
                auto bitmap = stored_bitmap.get();
                if (bitmap) {
                    auto size = p3d::Vec2i{(p3d::I_TYPE)bitmap->width, (p3d::I_TYPE)bitmap->height};
                    texture_init(&object->m_texture, size, (p3d::Pixel*) bitmap->data);
                    object->m_object.mesh = mesh;
                }
            }
        }
    }

    p3d::F_TYPE convert_scale_value(int32_t value) {
        return 0;
    }

    p3d::F_TYPE convert_rotation_value(int32_t value) {
        return 0;
    }

    p3d::F_TYPE convert_translation_value(int32_t value) {
        return 0;
    }

    // VDU 23, 0, &A0, sid; &48, 6, oid; scalex; :  Set Object X Scale Factor
    void set_object_x_scale_factor() {
        auto object = get_object();
        auto value = m_proc->readWord_t();
        if (object && (value >= 0)) {
            object->m_scale.x = convert_scale_value(value);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 7, oid; scaley; :  Set Object Y Scale Factor
    void set_object_y_scale_factor() {
        auto object = get_object();
        auto value = m_proc->readWord_t();
        if (object && (value >= 0)) {
            object->m_scale.y = convert_scale_value(value);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 8, oid; scalez; :  Set Object Z Scale Factor
    void set_object_z_scale_factor() {
        auto object = get_object();
        auto value = m_proc->readWord_t();
        if (object && (value >= 0)) {
            object->m_scale.y = convert_scale_value(value);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 9, oid; scalex; scaley; scalez :  Set Object XYZ Scale Factors
    void set_object_xyz_scale_factors() {
        auto object = get_object();
        auto valuex = m_proc->readWord_t();
        auto valuey = m_proc->readWord_t();
        auto valuez = m_proc->readWord_t();
        if (object && (valuex >= 0) && (valuey >= 0) && (valuez >= 0)) {
            object->m_scale.x = convert_scale_value(valuex);
            object->m_scale.y = convert_scale_value(valuey);
            object->m_scale.z = convert_scale_value(valuez);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 10, oid; anglex; :  Set Object X Rotation Angle
    void set_object_x_rotation_angle() {
        auto object = get_object();
        auto value = m_proc->readWord_t();
        if (object && (value >= 0)) {
            object->m_rotation.x = convert_rotation_value(value);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 11, oid; angley; :  Set Object Y Rotation Angle
    void set_object_y_rotation_angle() {
        auto object = get_object();
        auto value = m_proc->readWord_t();
        if (object && (value >= 0)) {
            object->m_rotation.y = convert_rotation_value(value);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 12, oid; anglez; :  Set Object Z Rotation Angle
    void set_object_z_rotation_angle() {
        auto object = get_object();
        auto value = m_proc->readWord_t();
        if (object && (value >= 0)) {
            object->m_rotation.z = convert_rotation_value(value);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 13, oid; anglex; angley; anglez; :  Set Object XYZ Rotation Angles
    void set_object_xyz_rotation_angles() {
        auto object = get_object();
        auto valuex = m_proc->readWord_t();
        auto valuey = m_proc->readWord_t();
        auto valuez = m_proc->readWord_t();
        if (object && (valuex >= 0) && (valuey >= 0) && (valuez >= 0)) {
            object->m_rotation.x = convert_rotation_value(valuex);
            object->m_rotation.y = convert_rotation_value(valuey);
            object->m_rotation.z = convert_rotation_value(valuez);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 14, oid; distx; :  Set Object X Translation Distance
    void set_object_x_translation_distance() {
        auto object = get_object();
        auto value = m_proc->readWord_t();
        if (object && (value >= 0)) {
            object->m_translation.x = convert_translation_value(value);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 15, oid; disty; :  Set Object Y Translation Distance
    void set_object_y_translation_distance() {
        auto object = get_object();
        auto value = m_proc->readWord_t();
        if (object && (value >= 0)) {
            object->m_translation.y = convert_translation_value(value);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 16, oid; distz; :  Set Object Z Translation Distance
    void set_object_z_translation_distance() {
        auto object = get_object();
        auto value = m_proc->readWord_t();
        if (object && (value >= 0)) {
            object->m_translation.z = convert_translation_value(value);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 17, oid; distx; disty; distz :  Set Object XYZ Translation Distances
    void set_object_xyz_translation_distances() {        auto object = get_object();
        auto valuex = m_proc->readWord_t();
        auto valuey = m_proc->readWord_t();
        auto valuez = m_proc->readWord_t();
        if (object && (valuex >= 0) && (valuey >= 0) && (valuez >= 0)) {
            object->m_translation.x = convert_translation_value(valuex);
            object->m_translation.y = convert_translation_value(valuey);
            object->m_translation.z = convert_translation_value(valuez);
        }
    }

    // VDU 23, 0, &A0, sid; &48, 18, bmid; :  Render To Bitmap
    void render_to_bitmap() {

    }

} Pingo3dControl;

#endif // PINGO_3D_H
