#include <easy3d/viewer/viewer.h>
#include <easy3d/core/surface_mesh.h>
#include <easy3d/renderer/drawable_lines.h>
#include <easy3d/renderer/renderer.h>
#include <easy3d/fileio/resources.h>
#include <easy3d/util/logging.h>


using namespace easy3d;


// This example shows how to
//		- rendering a vector field defined on a surface mesh;


int main(int argc, char **argv) {
    // Initialize logging.
    logging::initialize();

    // Create the default Easy3D viewer.
    // Note: a viewer must be created before creating any drawables.
    Viewer viewer("Tutorial_304_VectorField");

    // Load point cloud data from a file
    const std::string file_name = resource::directory() + "/data/sphere.obj";
    SurfaceMesh *model = dynamic_cast<SurfaceMesh *>(viewer.add_model(file_name, true));
    if (!model) {
        LOG(ERROR) << "Error: failed to load model. Please make sure the file exists and format is correct.";
        return EXIT_FAILURE;
    }

    // Get the bounding box of the model. Then we defined the length of the
    // normal vectors to be 5% of the bounding box diagonal.
    const Box3 &box = model->bounding_box();
    float length = norm(box.max() - box.min()) * 0.05f;

    // Compute the face normals.
    model->update_face_normals();
    auto normals = model->get_face_property<vec3>("f:normal");

    // Every consecutive two points represent a normal vector.
    std::vector<vec3> points;
    for (auto f : model->faces()) {
        vec3 center(0, 0, 0); // face center
        int count = 0;
        for (auto v : model->vertices(f)) {
            center += model->position(v);
            ++count;
        }

        const vec3 s = center / count;
        const vec3 t = s + normals[f] * length;
        points.push_back(s);
        points.push_back(t);
    }

    // Create a drawable for rendering the normal vectors.
    auto drawable = model->renderer()->add_lines_drawable("normals");
    // Upload the data to the GPU.
    drawable->update_vertex_buffer(points);

    // We will draw the normal vectors in a uniform green color
    drawable->set_uniform_coloring(vec4(0.0f, 1.0f, 0.0f, 1.0f));

    // Set the line width
    drawable->set_line_width(3.0f);

    // Also show the standard "edges"
    model->renderer()->get_lines_drawable("edges")->set_visible(true);

    // Run the viewer
    return viewer.run();
}

