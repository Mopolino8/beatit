/*
 ============================================================================

 .______    _______     ___   .___________.    __  .___________.
 |   _  \  |   ____|   /   \  |           |   |  | |           |
 |  |_)  | |  |__     /  ^  \ `---|  |----`   |  | `---|  |----`
 |   _  <  |   __|   /  /_\  \    |  |        |  |     |  |
 |  |_)  | |  |____ /  _____  \   |  |        |  |     |  |
 |______/  |_______/__/     \__\  |__|        |__|     |__|

 BeatIt - code for cardiovascular simulations
 Copyright (C) 2016 Simone Rossi

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ============================================================================
 */

/**
 * \file main.cpp
 *
 * \brief Here we test GetPot
 *
 * \author srossi
 *
 * \version 0.0
 *
 * Contact: srossi@gmail.com
 *
 * Created on: Aug 7, 2016
 *
 */

#include "libmesh/getpot.h"
#include "Util/IO/io.hpp"
// Functions to initialize the library.
#include "libmesh/libmesh.h"
// Basic include files needed for the mesh functionality.
#include "libmesh/mesh.h"

// Include file that defines (possibly multiple) systems of equations.
#include "libmesh/equation_systems.h"
// Include files that define a simple steady system
#include "libmesh/linear_implicit_system.h"
#include "libmesh/transient_system.h"
#include "libmesh/explicit_system.h"
#include "libmesh/vector_value.h"

//#include "libmesh/vtk_io.h"
#include "libmesh/exodusII_io.h"

// Define the Finite Element object.
#include "libmesh/fe.h"

// Define Gauss quadrature rules.
#include "libmesh/quadrature_gauss.h"

// Define useful datatypes for finite element
// matrix and vector components.
#include "libmesh/sparse_matrix.h"
#include "libmesh/numeric_vector.h"
#include "libmesh/dense_matrix.h"
#include "libmesh/dense_vector.h"
#include "libmesh/elem.h"
#include "libmesh/mesh_generation.h"

// Define the DofMap, which handles degree of freedom
// indexing.
#include "libmesh/dof_map.h"

#include "libmesh/zero_function.h"
#include "libmesh/dirichlet_boundaries.h"


#include "libmesh/error_vector.h"
#include "libmesh/kelly_error_estimator.h"
#include "libmesh/mesh_refinement.h"
#include "libmesh/fourth_error_estimators.h"

#include "libmesh/vtk_io.h"
// Function prototype.  This is the function that will assemble
// the linear system for our Poisson problem.  Note that the
// function will take the  EquationSystems object and the
// name of the system we are assembling as input.  From the
//  EquationSystems object we have access to the  Mesh and
// other objects we might need.
void assemble_poisson(
        libMesh::EquationSystems & es,
        const std::string & system_name);

void IC(libMesh::EquationSystems & es, const std::string & system_name);

int main(int argc, char ** argv)
{
    // Bring in everything from the libMesh namespace

    using namespace libMesh;
      // Initialize libraries, like in example 2.
      LibMeshInit init (argc, argv, MPI_COMM_WORLD);


      // Use the MeshTools::Generation mesh generator to create a uniform
      // 3D grid
      // We build a linear tetrahedral mesh (TET4) on  [0,2]x[0,0.7]x[0,0.3]
      // the number of elements on each side is read from the input file
      // Create a mesh, with dimension to be overridden later, on the
      // default MPI communicator.
      libMesh::Mesh mesh(init.comm());

      GetPot commandLine ( argc, argv );
      std::string datafile_name = commandLine.follow ( "data.beat", 2, "-i", "--input" );
      GetPot data(datafile_name);
      // allow us to use higher-order approximation.
      int numElementsX = data("mesh/elX", 1600);
      int numElementsY = data("mesh/elY",   5);
      int numElementsZ = data("mesh/elZ",   4);
      double maxX= data("mesh/maxX", 2.0);
      double maxY = data("mesh/maxY", 0.7);
      double maxZ = data("mesh/maxZ", 0.3);

      MeshTools::Generation::build_line ( mesh, numElementsX, 0., maxX );

    // Print information about the mesh to the screen.
    mesh.print_info();


    libMesh::EquationSystems systems(mesh);

    double mu = data("mu", 0.2);
    double dt = data("dt", 0.003125);
    typedef libMesh::Real Real;
    systems.parameters.set<Real>("mu") = mu;
    systems.parameters.set<Real>("dt") = dt;
    systems.parameters.set<Real>("dummy") = 42.0;
    systems.parameters.set<Real>("nobody") = 0.0;

    typedef libMesh::TransientLinearImplicitSystem Sys;
    Sys& Qsys = systems.add_system<Sys>("SimpleSystem");
    unsigned int Q_var = systems.get_system("SimpleSystem").add_variable("Q",
            libMesh::FIRST);

    Sys& Vsys = systems.add_system<Sys>("WaveSystem");
    unsigned int V_var = systems.get_system("WaveSystem").add_variable("V",
            libMesh::FIRST);

    Sys& ATsys = systems.add_system<Sys>("ATSystem");
    unsigned int at_var = systems.get_system("ATSystem").add_variable("at",
            libMesh::FIRST);


    systems.get_system("SimpleSystem").attach_assemble_function(
            assemble_poisson);
//    systems.get_system("SimpleSystem").attach_init_function (IC);



    // Initialize the data structures for the equation system.
    systems.init();

    *ATsys.solution = -1;
    auto index = Vsys.solution->first_local_index();

    Qsys.solution->set(index, 1);
    // Solve the system "Convection-Diffusion".  This will be done by
    // looping over the specified time interval and calling the
    // solve() member at each time step.  This will assemble the
    // system and call the linear solver.
    systems.get_system("SimpleSystem").time = 0.;


    // Prints information about the system to the screen.
    systems.print_info();

    // Write the system.
//    std::string output_system = data("output_systems", "");
//    if ("" != output_system)
//        systems.write(&output_system[0], libMesh::WRITE);
    // Write the output mesh if the user specified an
    // output file name.
//    std::string output_file = data("output_mesh", "");
//    if ("" != output_file)
//        mesh.write(&output_file[0]);

    // TIME LOOP
    std::string exodus_filename = "transient_ex1.e";
    libMesh::ExodusII_IO exo(mesh);

    exo.write_equation_systems (exodus_filename, systems);
    exo.append(true);
//    libMesh::VTKIO(mesh).write_equation_systems ("out.vtu.000", systems);


    unsigned int t_step = 0;
    for (;  systems.get_system("SimpleSystem").time < 42.0; t_step++)
    {
    	double time = systems.get_system("SimpleSystem").time;
        // Output evey 10 timesteps to file.
        exo.write_timestep (exodus_filename, systems, t_step+1, time);
//        std::ostringstream file_name;
//
//        file_name << "out.vtu."
//                  << std::setw(3)
//                  << std::setfill('0')
//                  << std::right
//                  << t_step+1;
//        libMesh::VTKIO(mesh).write_equation_systems (file_name.str(), systems);

        // Incremenet the time counter, set the time and the
        // time step size as parameters in the EquationSystem.
        systems.get_system("SimpleSystem").time += dt;

//        systems.parameters.set<libMesh::Real> ("time") = systems.get_system("SimpleSystem").time;
//        systems.parameters.set<libMesh::Real> ("dt")   = dt;

        // A pretty update message
//        libMesh::out << " Solving time step ";
//        // Do fancy zero-padded formatting of the current time.
//        {
//          std::ostringstream out;
//
//          out << std::setw(2)
//              << std::right
//              << t_step
//              << ", time="
//              << std::fixed
//              << std::setw(6)
//              << std::setprecision(3)
//              << std::setfill('0')
//              << std::left
//              << systems.get_system("SimpleSystem").time
//              <<  "...";
//
//          libMesh::out << out.str() << std::endl;
//        }

        // At this point we need to update the old
        // solution vector.  The old solution vector
        // will be the current solution vector from the
        // previous time step.  We will do this by extracting the
        // system from the EquationSystems object and using
        // vector assignment.  Since only TransientSystems
        // (and systems derived from them) contain old solutions
        // we need to specify the system type when we ask for it.
        *Qsys.old_local_solution = *Qsys.solution;
        *Vsys.old_local_solution = *Vsys.solution;

        Qsys.solve();
        // Define the refinement loop
        //Qsys.solution->print();


        *Vsys.solution = *Qsys.solution;
        *Vsys.solution *= dt;
        *Vsys.solution += *Vsys.old_local_solution;
        Vsys.update();

        auto first_local_index = Vsys.solution->first_local_index();
        auto last_local_index = Vsys.solution->last_local_index();
        for (auto i = first_local_index; i < last_local_index; ++i )
        {
        	double V = (*Vsys.solution)(i);
        	double at = (*ATsys.solution)(i);
        	if( V >= 0.9 && at == -1.0)
        	{
        		ATsys.solution->set(i, time);
        	}
        }

        if(t_step > 2000)
        {
        	break;
        }

        }
    // Output evey 10 timesteps to file.
//    libMesh::ExodusII_IO exo(mesh);
//    exo.append(true);
//    exo.write_timestep (exodus_filename, systems, t_step+1, time);

//	  libMesh::VTKIO (mesh).write_equation_systems ("out.pvtu", systems);
    // LibMeshInit object was created first, its destruction occurs
    // last, and it's destructor finalizes any external libraries and
    // checks for leaked memory.
    return 0;
}

// We now define the matrix assembly function for the
// Poisson system.  We need to first compute element
// matrices and right-hand sides, and then take into
// account the boundary conditions, which will be handled
// via a penalty method.
void assemble_poisson(
        libMesh::EquationSystems & es,
        const std::string & system_name)
{

    using libMesh::UniquePtr;
    // It is a good idea to make sure we are assembling
    // the proper system.
    libmesh_assert_equal_to(system_name, "SimpleSystem");

    double dt = es.parameters.get<double>("dt");
    double D = 1e-1; //es.parameters.get<double>("D");

    // Get a constant reference to the mesh object.
    const libMesh::MeshBase & mesh = es.get_mesh();

    // The dimension that we are running
    const unsigned int dim = mesh.mesh_dimension();

    // Get a reference to the LinearImplicitSystem we are solving
    libMesh::TransientLinearImplicitSystem & system = es.get_system<
            libMesh::TransientLinearImplicitSystem>("SimpleSystem");
	 system.rhs->zero();


    const unsigned int V_Var = system.variable_number("Q");

    // A reference to the  DofMap object for this system.  The  DofMap
    // object handles the index translation from node and element numbers
    // to degree of freedom numbers.  We will talk more about the  DofMap
    // in future examples.
    const libMesh::DofMap & dof_map = system.get_dof_map();

    // Get a constant reference to the Finite Element type
    // for the first (and only) variable in the system.
    libMesh::FEType fe_type = dof_map.variable_type(0);

    // Build a Finite Element object of the specified type.  Since the
    // FEBase::build() member dynamically creates memory we will
    // store the object as a UniquePtr<FEBase>.  This can be thought
    // of as a pointer that will clean up after itself.  Introduction Example 4
    // describes some advantages of  UniquePtr's in the context of
    // quadrature rules.
    UniquePtr<libMesh::FEBase> fe(libMesh::FEBase::build(dim, fe_type));

    // A 5th order Gauss quadrature rule for numerical integration.
    libMesh::QGauss qrule(dim, libMesh::SECOND);

    // Tell the finite element object to use our quadrature rule.
    fe->attach_quadrature_rule(&qrule);

    // Here we define some references to cell-specific data that
    // will be used to assemble the linear system.
    // The element Jacobian * quadrature weight at each integration point.
    const std::vector<libMesh::Real> & JxW = fe->get_JxW();
    // The physical XY locations of the quadrature points on the element.
    // These might be useful for evaluating spatially varying material
    // properties at the quadrature points.
    const std::vector<libMesh::Point> & q_point = fe->get_xyz();
    // The element shape functions evaluated at the quadrature points.
    const std::vector<std::vector<libMesh::Real> > & phi = fe->get_phi();
    // The element shape function gradients evaluated at the quadrature
    // points.
    const std::vector<std::vector<libMesh::RealGradient> > & dphi =
            fe->get_dphi();

    // Define data structures to contain the element matrix
    // and right-hand-side vector contribution.  Following
    // basic finite element terminology we will denote these
    // "Ke" and "Fe".  These datatypes are templated on
    //  Number, which allows the same code to work for real
    // or complex numbers.
    libMesh::DenseMatrix<libMesh::Number> Ke;
    libMesh::DenseVector<libMesh::Number> Fe;

    // This vector will hold the degree of freedom indices for
    // the element.  These define where in the global system
    // the element degrees of freedom get mapped.
    std::vector<libMesh::dof_id_type> dof_indices;

    // Now we will loop over all the elements in the mesh.
    // We will compute the element matrix and right-hand-side
    // contribution.
    libMesh::MeshBase::const_element_iterator el =
            mesh.active_local_elements_begin();
    const libMesh::MeshBase::const_element_iterator end_el =
            mesh.active_local_elements_end();

    // Loop over the elements.  Note that  ++el is preferred to
    // el++ since the latter requires an unnecessary temporary
    // object.
    for (; el != end_el; ++el)
    {
        // Store a pointer to the element we are currently
        // working on.  This allows for nicer syntax later.
        const libMesh::Elem * elem = *el;

        // Get the degree of freedom indices for the
        // current element.  These define where in the global
        // matrix and right-hand-side this element will
        // contribute to.
        dof_map.dof_indices(elem, dof_indices);

        // Compute the element-specific data for the current
        // element.  This involves computing the location of the
        // quadrature points (q_point) and the shape functions
        // (phi, dphi) for the current element.
        fe->reinit(elem);

        // Zero the element matrix and right-hand side before
        // summing them.  We use the resize member here because
        // the number of degrees of freedom might have changed from
        // the last element.  Note that this will be the case if the
        // element type is different (i.e. the last element was a
        // triangle, now we are on a quadrilateral).

        // The  DenseMatrix::resize() and the  DenseVector::resize()
        // members will automatically zero out the matrix  and vector.
        Ke.resize(dof_indices.size(), dof_indices.size());
        Fe.resize(dof_indices.size());

        // Now loop over the quadrature points.  This handles
        // the numeric integration.
        for (unsigned int qp = 0; qp < qrule.n_points(); qp++)
        {

            libMesh::Number V = 0;
            for(unsigned int i = 0; i < phi.size(); ++i )
            {
                V += phi[i][qp] * ( *system.old_local_solution)(dof_indices[i]);
            }

            // Now we will build the element matrix.  This involves
            // a double loop to integrate the test funcions (i) against
            // the trial functions (j).
            for (unsigned int i = 0; i < phi.size(); i++)
                for (unsigned int j = 0; j < phi.size(); j++)
                {
                    // Mass term
                    Ke(i, j) += JxW[qp] * (phi[i][qp] * phi[j][qp]);
                    // stiffness term
                    // Diffusion
                    Ke(i, j) += dt * D * JxW[qp] * (dphi[i][qp] * dphi[j][qp]);
                }

            // This is the end of the matrix summation loop
            // Now we build the element right-hand-side contribution.
            // This involves a single loop in which we integrate the
            // "forcing function" in the PDE against the test functions.
            {
//                const libMesh::Real R = 8 * V * (V - 1.0) * (0.1 - V);
                libMesh::Real R = -V;
                if(V>0.1) R+= 1.0;

                for (unsigned int i = 0; i < phi.size(); i++)
                {
                    // Mass term
                    Fe(i) += JxW[qp] * V * phi[i][qp];
                    // Reaction term
                    Fe(i) += dt * JxW[qp] * R * phi[i][qp];
                }
            }
        }
         // The element matrix and right-hand-side are now built
        // for this element.  Add them to the global matrix and
        // right-hand-side vector.  The  SparseMatrix::add_matrix()
        // and  NumericVector::add_vector() members do this for us.
        system.matrix->add_matrix(Ke, dof_indices);
        system.rhs->add_vector(Fe, dof_indices);
    }
    // All done!
}

libMesh::Number initial_value(
                                const libMesh::Point & p,
                                const libMesh::Parameters & parameters,
                                const std::string &,
                                const std::string &)
{
    if( 20 < p(0) ) return 1.0;
    else return 0.0;
}

void IC(libMesh::EquationSystems & es, const std::string & system_name)
{

    // Get a reference to the Convection-Diffusion system object.
    libMesh::TransientLinearImplicitSystem & system = es.get_system<
            libMesh::TransientLinearImplicitSystem>(system_name);

    // Project initial conditions at time 0
    es.parameters.set<libMesh::Real>("time") = system.time = 0;

    system.project_solution(initial_value, libmesh_nullptr, es.parameters);
}

