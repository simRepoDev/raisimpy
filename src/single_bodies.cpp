/**
 * Python wrappers for raisim.object using pybind11.
 *
 * Copyright (c) 2019, kangd and jhwangbo (original C++), Brian Delhaisse <briandelhaisse@gmail.com> (Python wrappers)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>     // automatic conversion between std::vector, std::list, etc to Python list/tuples/dict
#include <pybind11/eigen.h>   // automatic conversion between Eigen data types to Numpy data types
//#include <pybind11/numpy.h>   // numpy types

#include "raisim/math.hpp"   // contains the definitions of Vec, Mat, etc.
#include "raisim/object/singleBodies/Box.hpp"
#include "raisim/object/singleBodies/Capsule.hpp"
#include "raisim/object/singleBodies/Compound.hpp"
#include "raisim/object/singleBodies/Cone.hpp"
#include "raisim/object/singleBodies/Cylinder.hpp"
#include "raisim/object/singleBodies/Mesh.hpp"
#include "raisim/object/singleBodies/SingleBodyObject.hpp"
#include "raisim/object/singleBodies/Sphere.hpp"

#include "ode/collision.h"
#include "ode/ode.h"
#include "ode/extras/collision_kernel.h"
// Important note: for the above include ("ode/src/collision_kernel.h"), you have to add a `extras` folder in the
// `$LOCAL_BUILD/include/ode/` which should contain the following header files:
// array.h, collision_kernel.h, common.h, error.h, objects.h, odeou.h, odetls.h, threading_base.h, and typedefs.h.
// These header files can be found in the `ode/src` folder (like here: https://github.com/thomasmarsh/ODE/tree/master/ode/src)
//
// Why do we need to do that? The reason is that for `raisim::Mesh`, the authors of RaiSim use the `dSpaceID` variable
// type which has been forward declared in `ode/common.h` (but not defined there) as such:
//
// struct dxSpace;
// typedef struct dxSpace *dSpaceID;
//
// Thus for `dSpaceID` we need the definition of `dxSpace`, and this one is defined in `ode/src/collision_kernel.h` (in
// the `src` folder and not in the `include` folder!!). Pybind11 is looking for that definition, if you don't include
// it, pybind11 will complain and raise errors.


#include "converter.hpp"  // contains code that allows to convert between the Vec, Mat to numpy arrays.

namespace py = pybind11;
using namespace raisim;


void init_single_bodies(py::module &m) {


    /******************/
    /* GyroscopicMode */
    /******************/
    // GyroscopicMode enum (from include/raisim/object/singleBodies/SingleBodyObject.hpp)
    py::enum_<raisim::GyroscopicMode>(m, "GyroscopicMode", py::arithmetic())
	    .value("IMPLICIT_GYROSCOPIC_FORCE_BODY", raisim::GyroscopicMode::IMPLICIT_GYROSCOPIC_FORCE_BODY)  // implicit body model (stable, more computation)
	    .value("IMPLICIT_GYROSCOPIC_FORCE_WORLD", raisim::GyroscopicMode::IMPLICIT_GYROSCOPIC_FORCE_WORLD)  // implicit world model (stable, more computation)
	    .value("EXPLICIT_GYROSCOPIC_FORCE", raisim::GyroscopicMode::EXPLICIT_GYROSCOPIC_FORCE)  // explicit model (unstable, less computation)
	    .value("NO_GYROSCOPIC_FORCE", raisim::GyroscopicMode::NO_GYROSCOPIC_FORCE);


    /********************/
	/* SingleBodyObject */
	/********************/
	// SingleBodyObject class (from include/raisim/object/singleBodies/SingleBodyObject.hpp)
	py::class_<raisim::SingleBodyObject, raisim::Object>(m, "SingleBodyObject", "Raisim Single Object from which all single objects/bodies (such as box, sphere, etc) inherit from.")

	    .def(py::init<raisim::ObjectType>(), "Initialize the Object.", py::arg("object_type"))


        .def("get_quaternion", py::overload_cast<>(&raisim::SingleBodyObject::getQuaternion), R"mydelimiter(
	    Get the body's orientation (expressed as a quaternion [w,x,y,z]) with respect to the world frame.

	    Returns:
	        np.array[float[4]]: quaternion [w,x,y,z].
	    )mydelimiter")


	    .def("get_rotation_matrix", py::overload_cast<>(&raisim::SingleBodyObject::getRotationMatrix), R"mydelimiter(
	    Get the body's orientation (expressed as a rotation matrix) with respect to the world frame.

	    Returns:
	        np.array[float[3,3]]: rotation matrix.
	    )mydelimiter")


	    .def("get_position", py::overload_cast<>(&raisim::SingleBodyObject::getPosition), R"mydelimiter(
	    Get the body's position with respect to the world frame.

	    Returns:
	        np.array[float[3]]: position in the world frame.
	    )mydelimiter")


	    .def("get_com_position", &raisim::SingleBodyObject::getComPosition, R"mydelimiter(
	    Get the body's center of mass position with respect to the world frame.

	    Returns:
	        np.array[float[3]]: center of mass position in the world frame.
	    )mydelimiter")


	    .def("get_linear_velocity", py::overload_cast<>(&raisim::SingleBodyObject::getLinearVelocity), R"mydelimiter(
	    Get the body's linear velocity with respect to the world frame.

	    Returns:
	        np.array[float[3]]: linear velocity in the world frame.
	    )mydelimiter")


	    .def("get_angular_velocity", py::overload_cast<>(&raisim::SingleBodyObject::getAngularVelocity), R"mydelimiter(
	    Get the body's angular velocity position with respect to the world frame.

	    Returns:
	        np.array[float[3]]: angular velocity in the world frame.
	    )mydelimiter")


        // this is the same as get_position
        .def("get_world_position", py::overload_cast<>(&raisim::SingleBodyObject::getPosition), R"mydelimiter(
	    Get the body's position with respect to the world frame.

	    Returns:
	        np.array[float[3]]: position in the world frame.
	    )mydelimiter")


	    // this is the same as get_rotation_matrix
	    .def("get_world_rotation_matrix", py::overload_cast<>(&raisim::SingleBodyObject::getRotationMatrix), R"mydelimiter(
	    Get the body's orientation (expressed as a rotation matrix) with respect to the world frame.

	    Returns:
	        np.array[float[3,3]]: rotation matrix.
	    )mydelimiter")


	    .def("get_kinetic_energy", &raisim::SingleBodyObject::getKineticEnergy, R"mydelimiter(
	    Get the body's kinetic energy.

	    Returns:
	        float: kinetic energy.
	    )mydelimiter")


	    .def("get_potential_energy", [](raisim::SingleBodyObject &self, py::array_t<double> gravity) {
            Vec<3> g = convert_np_to_vec<3>(gravity);
	        return self.getPotentialEnergy(g);
	    }, R"mydelimiter(
	    Get the body's potential energy due to gravity.

	    Args:
	        gravity (np.array[float[3]]): gravity vector.

	    Returns:
	        float: potential energy.
	    )mydelimiter",
	    py::arg("gravity"))


	    .def("get_energy", [](raisim::SingleBodyObject &self, py::array_t<double> gravity) {
            Vec<3> g = convert_np_to_vec<3>(gravity);
	        return self.getEnergy(g);
	    }, R"mydelimiter(
	    Get the body's total energy.

	    Args:
	        gravity (np.array[float[3]]): gravity vector.

	    Returns:
	        float: total energy.
	    )mydelimiter",
	    py::arg("gravity"))


	    .def("get_linear_momentum", &raisim::SingleBodyObject::getLinearMomentum, R"mydelimiter(
	    Get the body's linear momentum.

	    Returns:
	        np.array[float[3]]: linear momentum.
	    )mydelimiter")


	    .def("get_mass", &raisim::SingleBodyObject::getMass, R"mydelimiter(
	    Get the body's mass.

	    Args:
	        local_idx (int): local index.

	    Returns:
	        float: mass (kg).
	    )mydelimiter",
	    py::arg("local_idx"))


	    .def("get_world_inertia_matrix", &raisim::SingleBodyObject::getInertiaMatrix_W, R"mydelimiter(
	    Get the body's inertia matrix expressed in the world frame.

	    Returns:
	        np.array[float[3,3]]: world inertia matrix.
	    )mydelimiter")


	    .def("get_body_inertia_matrix", &raisim::SingleBodyObject::getInertiaMatrix_B, R"mydelimiter(
	    Get the body's inertia matrix expressed in the body frame.

	    Returns:
	        np.array[float[3,3]]: body inertia matrix.
	    )mydelimiter")


	    .def("get_object_type", &raisim::SingleBodyObject::getObjectType, R"mydelimiter(
	    Get the body's type.

	    Returns:
	        raisim.ObjectType: object type (BOX, CYLINDER, CAPSULE, CONE, SPHERE, etc.)
	    )mydelimiter")


        .def("get_collision_object", &raisim::SingleBodyObject::getCollisionObject, R"mydelimiter(
	    Get the collision object.

	    Returns:
	        dGeomID: collision object.
	    )mydelimiter")


        .def("get_gyroscopic_mode", &raisim::SingleBodyObject::getCollisionObject, R"mydelimiter(
	    Get the gyroscopic mode.

	    Returns:
	        raisim.GyroscopicMode: gyroscopic mode (between ['IMPLICIT_GYROSCOPIC_FORCE_BODY',
	        'IMPLICIT_GYROSCOPIC_FORCE_WORLD', 'EXPLICIT_GYROSCOPIC_FORCE', 'NO_GYROSCOPIC_FORCE'])
	    )mydelimiter")


        .def("set_position", py::overload_cast<const Eigen::Vector3d &>(&raisim::SingleBodyObject::setPosition), R"mydelimiter(
	    Set the specified origin position.

	    Args:
	        origin_position (np.array[float[3]]): origin position.
	    )mydelimiter",
	    py::arg("origin_position"))


	    .def("set_position", py::overload_cast<double, double, double>(&raisim::SingleBodyObject::setPosition), R"mydelimiter(
	    Set the specified origin position.

	    Args:
	        x (float): x component of the origin position.
	        y (float): y component of the origin position.
	        z (float): z component of the origin position.
	    )mydelimiter",
	    py::arg("x"), py::arg("y"), py::arg("z"))


        .def("set_orientation", py::overload_cast<double, double, double, double>(&raisim::SingleBodyObject::setOrientation), R"mydelimiter(
	    Set the specified orientation (expressed as a quaternion [x,y,z,w]) for the body.

	    Args:
	        w (float): scalar component of the quaternion.
	        x (float): x component of the vector in the quaternion.
	        y (float): y component of the vector in the quaternion.
	        z (float): z component of the vector in the quaternion.
	    )mydelimiter",
	    py::arg("w")=1.0, py::arg("x")=0.0, py::arg("y")=0.0, py::arg("z")=0.0)


        .def("set_orientation", py::overload_cast<const Eigen::Matrix3d &>(&raisim::SingleBodyObject::setOrientation), R"mydelimiter(
	    Set the specified orientation (expressed as a rotation matrix) for the body.

	    Args:
	        rotation_matrix (np.array[float[3,3]]): rotation matrix.
	    )mydelimiter",
	    py::arg("rotation_matrix"))


        .def("set_orientation", [](raisim::SingleBodyObject &self, py::array_t<double> quaternion) {
            Eigen::Quaterniond quat = convert_np_to_quaternion(quaternion);
            self.setOrientation(quat);
        }, R"mydelimiter(
	    Set the specified orientation (expressed as a quaternion [x,y,z,w]) for the body.

	    Args:
	        quaternion (np.array[float[4]]): quaternion [x,y,z,w].
	    )mydelimiter",
	    py::arg("quaternion"))


        .def("set_pose", py::overload_cast<const Eigen::Vector3d &, const Eigen::Matrix3d &>(&raisim::SingleBodyObject::setPose), R"mydelimiter(
	    Set the specified pose for the body.

	    Args:
	        position (np.array[float[3]]): origin position vector.
	        rotation_matrix (np.array[float[3,3]]): rotation matrix.
	    )mydelimiter",
	    py::arg("position"), py::arg("rotation_matrix"))


	    .def("set_pose", [](raisim::SingleBodyObject &self, Eigen::Vector3d position, py::array_t<double> quaternion) {
	        Eigen::Quaterniond quat = convert_np_to_quaternion(quaternion);
            self.setPose(position, quat);
	    }, R"mydelimiter(
	    Set the specified pose for the body.

	    Args:
	        position (np.array[float[3]]): origin position vector.
	        quaternion (np.array[float[4]]): quaternion (expressed as [w,x,y,z]).
	    )mydelimiter",
	    py::arg("position"), py::arg("quaternion"))


        .def("set_velocity", py::overload_cast<const Eigen::Vector3d &, const Eigen::Vector3d &>(&raisim::SingleBodyObject::setVelocity), R"mydelimiter(
	    Set the specified linear and angular velocities for the body.

	    Args:
	        linear_velocity (np.array[float[3]]): linear velocity.
	        angular_velocity (np.array[float[3,3]]): angular velocity.
	    )mydelimiter",
	    py::arg("linear_velocity"), py::arg("angular_velocity"))


        .def("set_velocity", py::overload_cast<double, double, double, double, double, double>(&raisim::SingleBodyObject::setVelocity), R"mydelimiter(
	    Set the specified linear and angular velocities for the body.

	    Args:
	        dx (float): x component of the linear velocity.
	        dy (float): y component of the linear velocity.
	        dz (float): z component of the linear velocity.
	        wx (float): x component of the angular velocity.
	        wy (float): y component of the angular velocity.
	        wz (float): z component of the angular velocity.
	    )mydelimiter",
	    py::arg("dx"), py::arg("dy"), py::arg("dz"), py::arg("wx"), py::arg("wy"), py::arg("wz"))


        .def("set_external_force", [](raisim::SingleBodyObject &self, size_t local_idx, py::array_t<double> force) {
	        // convert np.array[3] to Vec<3>
	        Vec<3> f = convert_np_to_vec<3>(force);
	        self.setExternalForce(local_idx, f);
	    }, R"mydelimiter(
	    Set the external force on the body.

	    Args:
	        local_idx (int): local index.
	        force (np.array[float[3]]): force vector.
	    )mydelimiter",
	    py::arg("local_idx"), py::arg("force"))


	    .def("set_external_torque", [](raisim::SingleBodyObject &self, size_t local_idx, py::array_t<double> torque) {
	        // convert np.array[3] to Vec<3>
	        Vec<3> t = convert_np_to_vec<3>(torque);
	        self.setExternalTorque(local_idx, t);
	    }, R"mydelimiter(
	    Set the external force on the body.

	    Args:
	        local_idx (int): local index.
	        force (np.array[float[3]]): force vector.
	    )mydelimiter",
	    py::arg("local_idx"), py::arg("torque"))


        .def("set_gyroscopic_mode", &raisim::SingleBodyObject::setGyroscopicMode, R"mydelimiter(
	    Set the gyroscopic mode for the body.

	    Args:
	        mode (GyroscopicMode): gyroscopic mode (select between (between [GyroscopicMode.IMPLICIT_GYROSCOPIC_FORCE_BODY,
	            GyroscopicMode.IMPLICIT_GYROSCOPIC_FORCE_WORLD, GyroscopicMode.EXPLICIT_GYROSCOPIC_FORCE,
	            GyroscopicMode.NO_GYROSCOPIC_FORCE])
	    )mydelimiter",
	    py::arg("mode"))


        .def("pre_contact_solver_update1", [](raisim::SingleBodyObject &self, py::array_t<double> gravity, double dt) {
	        // convert np.array[3] to Vec<3>
	        Vec<3> vec = convert_np_to_vec<3>(gravity);
	        self.preContactSolverUpdate1(vec, dt);
	    }, py::arg("gravity"), py::arg("dt"))


	    .def("pre_contact_solver_update2", [](raisim::SingleBodyObject &self, py::array_t<double> gravity, double dt) {
	        // convert np.array[3] to Vec<3>
	        Vec<3> vec = convert_np_to_vec<3>(gravity);
	        self.preContactSolverUpdate1(vec, dt);
	    }, py::arg("gravity"), py::arg("dt"))


	    .def("integrate", &raisim::SingleBodyObject::integrate, "integrate.", py::arg("dt"))


        .def("get_contact_point_velocity", [](raisim::SingleBodyObject &self, size_t point_id) {
	        Vec<3> vel;
	        self.getContactPointVel(point_id, vel);
	        return convert_vec_to_np(vel);
	    }, R"mydelimiter(
	    Get the contact point velocity.

	    Args:
	        point_id (int): point id.

	    Returns:
	        np.array[float[3]]: contact point velocity.
	    )mydelimiter",
	    py::arg("point_id"))


        .def("update_collision", &raisim::SingleBodyObject::updateCollision, "Update the collisions.")


        .def("set_linear_damping", &raisim::SingleBodyObject::setLinearDamping, R"mydelimiter(
	    Set the body's linear damping coefficient.

	    Args:
	        damping (float): linear damping coefficient.
	    )mydelimiter",
	    py::arg("damping"))


	    .def("set_angular_damping", [](raisim::SingleBodyObject &self, py::array_t<double> damping) {
            // convert np.array[3] to Vec<3>
	        Vec<3> vec = convert_np_to_vec<3>(damping);
	        self.setAngularDamping(vec);
	    }, R"mydelimiter(
	    Set the body's angular damping.

	    Args:
	        damping (np.array[float[3]]): angular damping.
	    )mydelimiter",
	    py::arg("damping"))


	    .def("set_body_type", &raisim::SingleBodyObject::setBodyType, R"mydelimiter(
	    Set the body's type.

	    Args:
	        body_type (BodyType): body type.
	    )mydelimiter",
	    py::arg("body_type"))


	    .def("get_collision_group", &raisim::SingleBodyObject::getCollisionGroup, R"mydelimiter(
	    Get the body's collision group.

	    Returns:
	        int: collision group.
	    )mydelimiter")

	    .def("get_collision_mask", &raisim::SingleBodyObject::getCollisionMask, R"mydelimiter(
	    Get the body's collision mask.

	    Returns:
	        int: collision mask.
	    )mydelimiter")
	    ;


    /*******/
	/* Box */
	/*******/
	// Box class (from include/raisim/object/singleBodies/Box.hpp)
	py::class_<raisim::Box, raisim::SingleBodyObject>(m, "Box", "Raisim Box.")
	    .def(py::init<double, double, double, double>(),
	    "Initialize a box.\n\n"
	    "Args:\n"
	    "    x (float): length along the x axis.\n"
	    "    y (float): length along the y axis.\n"
	    "    z (float): length along the z axis.\n"
	    "    mass (float): mass of the box.",
	    py::arg("x"), py::arg("y"), py::arg("z"), py::arg("mass"))
	    .def("get_dimensions", [](raisim::Box &box) {
	        Vec<3> dimensions = box.getDim();
	        return convert_vec_to_np(dimensions);
	    }, R"mydelimiter(
	    Get the box's dimensions.

	    Returns:
	        tuple[float[3]]: dimensions along each axis.
	    )mydelimiter");


    /***********/
	/* Capsule */
	/***********/
	// Capsule class (from include/raisim/object/singleBodies/Capsule.hpp)
	py::class_<raisim::Capsule, raisim::SingleBodyObject>(m, "Capsule", "Raisim Capsule.")
	    .def(py::init<double, double, double>(),
	    "Initialize a capsule.\n\n"
	    "Args:\n"
	    "    radius (float): radius of the capsule.\n"
	    "    height (float): height of the capsule.\n"
	    "    mass (float): mass of the capsule.",
	    py::arg("radius"), py::arg("height"), py::arg("mass"))
	    .def("get_radius", &raisim::Capsule::getRadius, R"mydelimiter(
	    Get the capsule's radius.

	    Returns:
	        float: radius of the capsule.
	    )mydelimiter")
	    .def("get_height", &raisim::Capsule::getHeight, R"mydelimiter(
	    Get the capsule's height.

	    Returns:
	        float: height of the capsule.
	    )mydelimiter");


    /************/
    /* Compound */
    /************/
    // Compound class (from include/raisim/object/singleBodies/Compound.hpp)
    py::class_<raisim::Compound, raisim::SingleBodyObject> compound(m, "Compound", "Raisim Compound bodies.");

    py::class_<raisim::Compound::CompoundObjectChild>(compound, "CompoundObjectChild", "Raisim Compound object child.")
        .def_readwrite("object_type", &raisim::Compound::CompoundObjectChild::objectType)
        .def_property("object_param",
            [](raisim::Compound::CompoundObjectChild &self) { // getter
                // convert from Vec<4> to np.array
                return convert_vec_to_np(self.objectParam);
            }, [](raisim::Compound::CompoundObjectChild &self, py::array_t<double> param) { // setter
                // convert from np.array to Vec<4>
                Vec<4> vec = convert_np_to_vec<4>(param);
                self.objectParam = vec;
            })
        .def_readwrite("material", &raisim::Compound::CompoundObjectChild::material)
        .def_readwrite("transformation", &raisim::Compound::CompoundObjectChild::trans);

    compound.def(py::init([](const std::vector<raisim::Compound::CompoundObjectChild>& children, double mass,
        py::array_t<double> inertia) {
        // convert np.array to Mat<3,3>
        Mat<3, 3> I = convert_np_to_mat<3, 3>(inertia);
        return new Compound(children, mass, I);
    }), R"mydelimiter(
    Initialize the Compound object.

    Args:
        children (list[CompoundObjectChild]): list of child objects.
        mass (float): total mass of the compound object.
        inertia (np.array[float[3,3]]): total inertia matrix of the compound object.

    )mydelimiter",
    py::arg("children"), py::arg("mass"), py::arg("inertia"));


    /********/
	/* Cone */
	/********/
	// Cone class (from include/raisim/object/singleBodies/Cone.hpp)
	py::class_<raisim::Cone, raisim::SingleBodyObject>(m, "Cone", "Raisim Cone.")
	    .def("get_radius", &raisim::Cone::getRadius, R"mydelimiter(
	    Get the cone's radius.

	    Returns:
	        float: radius of the cone.
	    )mydelimiter")
	    .def("get_height", &raisim::Cone::getHeight, R"mydelimiter(
	    Get the cone's height.

	    Returns:
	        float: height of the cone.
	    )mydelimiter");


    /************/
    /* Cylinder */
    /************/
    // Cylinder class (from include/raisim/object/singleBodies/Cylinder.hpp)
	py::class_<raisim::Cylinder, raisim::SingleBodyObject>(m, "Cylinder", "Raisim Cylinder.")
	    .def("get_radius", &raisim::Cylinder::getRadius, R"mydelimiter(
	    Get the cylinder's radius.

	    Returns:
	        float: radius of the cylinder.
	    )mydelimiter")
	    .def("get_height", &raisim::Cylinder::getHeight, R"mydelimiter(
	    Get the cylinder's height.

	    Returns:
	        float: height of the cylinder.
	    )mydelimiter");


    /********/
    /* Mesh */
    /********/
    // Mesh class (from include/raisim/object/singleBodies/Mesh.hpp)
	py::class_<raisim::Mesh, raisim::SingleBodyObject>(m, "Mesh", "Raisim Mesh.")
        .def("get_file_name", &raisim::Mesh::getMeshFileName, R"mydelimiter(
	    Get the mesh's full path.

	    Returns:
	        float: full path of the mesh file.
	    )mydelimiter");

    /**********/
	/* Sphere */
	/**********/
	// Sphere class (from include/raisim/object/singleBodies/Sphere.hpp)
	py::class_<raisim::Sphere, raisim::SingleBodyObject>(m, "Sphere", "Raisim Sphere.")
        .def("get_radius", &raisim::Sphere::getRadius, R"mydelimiter(
	    Get the sphere's radius.

	    Returns:
	        float: radius of the sphere.
	    )mydelimiter");

}