#include "../MyMisc.h"
#include <direct.h>
#include <easy3d/viewer/viewer.h>
#include <easy3d/core/surface_mesh.h>
#include <easy3d/renderer/drawable_lines.h>
#include <easy3d/renderer/renderer.h>
#include <easy3d/fileio/resources.h>
#include <easy3d/util/logging.h>
#include <easy3d/util/file_system.h>
#include "../wave.h"
#include "../Power2Distribution.h"

using namespace easy3d;

struct MyModel : public Model
{
    MyModel()
    {
        m_world.initialize();
        m_world.readPoints((std::vector<float3>&)m_points);
    }

    /** The vertices of the model. */
    virtual const std::vector<vec3>& points() const
    {
        return m_points;
    }

    /** Test if the model is empty. */
    bool empty() const
    {
        return points().empty();
    };

    /** prints the names of all properties to an output stream (e.g., std::cout). */
    virtual void property_stats(std::ostream& output) const
    {
        nvAssert(false);
    }

    void makeSimulationStep() { m_world.makeSimulationStep(); }

private:
    std::vector<vec3> m_points;
    World m_world;
};

struct MyViewer : public Viewer
{
    MyViewer(const char* sName, MyModel *pModel) : Viewer(sName), m_pModel(pModel) { }
    virtual void pre_draw()
    {
        m_pModel->makeSimulationStep();
        Viewer::pre_draw();
    }

private:
    MyModel* m_pModel;
};

int main(int argc, char** argv)
{
    nvAssert(Power2Distribution::dbgDoesTestPass());

    // initialize logging
    logging::initialize();

    // find directory with resources
    std::string dir = file_system::executable_directory();
    for (; ; )
    {
        if (file_system::is_directory(dir + "/Easy3D"))
        {
            _chdir((dir + "/Easy3D/resources").c_str());
            break;
        }
        dir = file_system::parent_directory(dir);
    }

    MyModel* pMyModel = new MyModel;

    // Create the default Easy3D viewer.
    // Note: a viewer must be created before creating any drawables.
    MyViewer viewer("atom", pMyModel);

    // Load point cloud data from a file
    viewer.add_model(pMyModel, false);

    // Get the bounding box of the model. Then we defined the length of the
    // normal vectors to be 5% of the bounding box diagonal.
    const Box3& box = pMyModel->bounding_box();
    float length = box.diagonal() * 0.05f;

    // Create a drawable for rendering the normal vectors.
    auto drawable = pMyModel->renderer()->add_lines_drawable("normals");
    // Upload the data to the GPU.
    drawable->update_vertex_buffer(pMyModel->points());

    // We will draw the normal vectors in a uniform green color
    drawable->set_uniform_coloring(vec4(1.0f, 0.0f, 0.0f, 1.0f));

    // Set the line width
    drawable->set_line_width(3.0f);

    // Run the viewer
    return viewer.run();
}