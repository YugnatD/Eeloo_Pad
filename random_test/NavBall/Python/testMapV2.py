from mayavi import mlab
from tvtk.api import tvtk # python wrappers for the C++ vtk ecosystem

def auto_sphere(image_file):
    pitch = 10.0  # degrees
    roll = -5.0  # degrees
    heading = 120.0  # degrees
    # create a figure window (and scene)
    fig = mlab.figure(size=(600, 600))

    # load and map the texture
    img = tvtk.JPEGReader()
    img.file_name = image_file
    texture = tvtk.Texture(input_connection=img.output_port, interpolate=1)
    # (interpolate for a less raster appearance when zoomed in)

    # use a TexturedSphereSource, a.k.a. getting our hands dirty
    R = 1
    Nrad = 180

    # create the sphere source with a given radius and angular resolution
    sphere = tvtk.TexturedSphereSource(radius=R, theta_resolution=Nrad,
                                       phi_resolution=Nrad)

    # assemble rest of the pipeline, assign texture    
    sphere_mapper = tvtk.PolyDataMapper(input_connection=sphere.output_port)
    sphere_actor = tvtk.Actor(mapper=sphere_mapper, texture=texture)
    fig.scene.add_actor(sphere_actor)
    # save the projection of the sphere to a file according to the current pitch, roll, and heading


if __name__ == "__main__":
    # image_file = 'blue_marble_spherical.jpg'
    image_file = 'NavBall_Texture.jpg'
    # image_file = 'NavBall_Texture.png'

    auto_sphere(image_file)
    mlab.show()