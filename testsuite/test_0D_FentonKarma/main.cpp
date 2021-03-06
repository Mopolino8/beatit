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
 * \class main
 *
 * \brief This test check the Nash Panfilov Ionic Model
 *
 *
 * \author srossi
 *
 * \version 0.0
 *
 *
 * Contact: srossi@gmail.com
 *
 * Created on: Aug 11, 2016
 *
 */

#include <memory>
#include "Electrophysiology/IonicModels/FentonKarma.hpp"
#include "Util/IO/io.hpp"
#include "Util/CTestUtil.hpp"
#include <iomanip>

#include "libmesh/getpot.h"



// Start the main Program
int main (int argc, char ** argv)
{
    // Print BeatIt on screen
    BeatIt::printBanner(std::cout);
	// Read the input file
	GetPot commandLine(argc, argv);
    std::string datafile_name = commandLine.follow ( "data.beat", 2, "-i", "--input" );
    GetPot data(datafile_name);

    // Create a the Nas-Panfilov model using the Ionic Model factory
	std::unique_ptr<BeatIt::FentonKarma> pNP( new BeatIt::FentonKarma() );
	pNP->setup(data, "model");

	// Check the number of variables in the model
	int numVar = pNP->numVariables();
	// Create a container to store the variables of the ionic model
	std::vector<double> variables(numVar, 0.0);

    std::vector<double> old_variables(numVar, 0.0);
    std::vector<double> older_variables(numVar, 0.0);
    std::vector<double> rhs(numVar, 0.0);
    std::vector<double> old_rhs(numVar, 0.0);

    // Create the output file
	std::ofstream output("results.txt");
    // Initialize the output file
	pNP->initializeSaveData(output);
    // set the default initial conditions in the ionic model
	pNP->initialize(variables);

	// timestep
	double dt = 0.125;
    // output at every save_iter
	int save_iter = 0.05 / dt;
	// Final time
	double TF = 500.0;
	// External Stimulus
	double Ist = 0;
	// Current time
	double time = 0.0;
	// save to output the initial data
	BeatIt::saveData(time, variables, output);
	// I don't remember why we use this (may be we do not use it ...)
	int iter = 0;

	std::cout << "v0 = " << variables[0] << std::endl;
	    // for ctest purposes
    double solution_norm = 0.0;

    double Iion = 0;
    double Iion_old = 0.0;
    // loop over time
    variables[0] = 1.0;
    while( time <= TF )
	{

		if(time >= 0. && time <= 0.5)
		{
			Ist = -0.0;
		}
		else Ist = 0.0;
        for(int k = 0; k < variables.size(); ++k )
        {
            older_variables[k] = old_variables[k];
            old_variables[k] = variables[k];
            old_rhs[k] = rhs[k];
        }
        Iion_old = Iion;

        // solve one timestep
        //pNP->solve(variables, Ist, dt);

        if(iter == 0)
		//if(true)
		{
		    pNP->updateVariables(variables, rhs, Ist, dt, false);
		    for(int k = 1; k < variables.size(); ++k )
		    {
		        variables[k] += dt * rhs[k];
		    }
            Iion = pNP->evaluateIonicCurrent(variables, Ist, dt);
            variables[0] -= dt * (Iion + Ist);

		}
		else
		{
            pNP->updateVariables(variables, rhs, Ist, dt, false);
            for(int k = 1; k < variables.size(); ++k )
            {
                variables[k] = 4.0/3.0*old_variables[k]-1.0/3.0*older_variables[k]
                             + 2.0 / 3.0 * dt * (2*rhs[k]-old_rhs[k]);
            }
            Iion = pNP->evaluateIonicCurrent(variables, Ist, dt);
            variables[0] = 4.0/3.0*old_variables[0]-1.0/3.0*older_variables[0] - 2.0 / 3.0 * dt * (2*Iion -Iion_old+ Ist);

		}


		// update the current time
		time += dt;
		// update current iteration
		++iter;
		// if we want to output we can
		if( 0 == iter%save_iter ) BeatIt::saveData(time, variables, output);
        // for ctest purposes
        solution_norm += variables[0];
	}
    // close output file
	output.close();

    std::cout << std::setprecision(18) << "Solution norm = " << solution_norm << std::endl;
    //up to the 16th digit
    const double reference_solution_norm = 2814.72668772403449;
    //We check only up to 12th
	return BeatIt::CTest::check_test(solution_norm, reference_solution_norm, 1e-8);
}

