/*-------------FDTD.cpp-------------------------------------------------------//
*
*              Finite Difference Time Domain
*
* Purpose: To replicate the results of our invisible lense raytracer with 
*          FDTD. Woo!
*
*   Notes: Most of this is coming from the following link:
*             http://www.eecs.wsu.edu/~schneidj/ufdtd/chap3.pdf
*             http://www.eecs.wsu.edu/~schneidj/ufdtd/chap8.pdf
*
*-----------------------------------------------------------------------------*/

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

static const size_t space = 300;

struct Bound{
    int x,y;
};

struct Loss{
    std::vector <double> EzH = std::vector<double>(space * space, 0), 
                         EzE = std::vector<double>(space * space, 0), 
                         HyE = std::vector<double>(space * space, 0), 
                         HyH = std::vector<double>(space * space, 0),
                         HxE = std::vector<double>(space * space, 0), 
                         HxH = std::vector<double>(space * space, 0);
};

struct Loss1d{
    std::vector <double> EzH = std::vector<double>(space, 0), 
                         EzE = std::vector<double>(space, 0), 
                         HyE = std::vector<double>(space, 0), 
                         HyH = std::vector<double>(space, 0);
};

struct Field{
    std::vector <double> Hx = std::vector<double>(space * space, 0), 
                         Hy = std::vector<double>(space * space, 0),
                         Ez = std::vector<double>(space * space, 0);

    std::vector <double> Hy1d = std::vector<double>(space, 0), 
                         Ez1d = std::vector<double>(space, 0);

    // 6 elements, 3 spacial elements away from border and 2 time elements of
    // those spatial elements
    std::vector <double> Etop = std::vector<double>(3 * 2 * space, 0), 
                         Ebot = std::vector<double>(3 * 2 * space, 0),
                         Eleft = std::vector<double>(3 * 2 * space, 0),
                         Eright = std::vector<double>(3 * 2 * space, 0);

    int t;
};

#define EzH(i, j) EzH[(i) + (j) *  space]
#define EzE(i, j) EzE[(i) + (j) *  space]
#define HyH(i, j) HyH[(i) + (j) *  space]
#define HyE(i, j) HyE[(i) + (j) *  space]
#define HxH(i, j) HxH[(i) + (j) *  space]
#define HxE(i, j) HxE[(i) + (j) *  space]
#define Hx(i, j) Hx[(i) + (j) *  space] 
#define Hy(i, j) Hy[(i) + (j) *  space] 
#define Ez(i, j) Ez[(i) + (j) *  space] 
#define Etop(k, j, i) Etop[(i) * 6 + (j) * 3 + (k)]
#define Ebot(k, j, i) Ebot[(i) * 6 + (j) * 3 + (k)]
#define Eleft(i, j, k) Eleft[(k) * 6 + (j) * 3 + (i)]
#define Eright(i, j, k) Eright[(k) * 6 + (j) * 3 + (i)]


void FDTD(Field EM,
          const int final_time, const double eps, const int space,
          std::ofstream& output);

// Adding ricker solutuion
double ricker(int time, int loc, double Cour);

// Adding plane wave
double planewave(int time, int loc, double Cour, int ppw, double radius);

// 2 dimensional functions for E / H movement
Field Hupdate2d(Field EM, Loss lass, int t);
Field Eupdate2d(Field EM, Loss lass, int t);

// 1 dimensional update functions for E / H
Field Hupdate1d(Field EM, Loss1d lass1d, int t);
Field Eupdate1d(Field EM, Loss1d lass1d, int t);

// Creating loss
Loss createloss2d(Loss lass, double eps, double Cour, double loss);
Loss1d createloss1d(Loss1d lass1d, double eps, double Cour, double loss);

// Total Field Scattered Field (TFSF) boundaries
Field TFSF(Field EM, Loss lass, Loss1d lass1d, double Cour);

// Checking Absorbing Boundary Conditions (ABS)
Field ABCcheck(Field EM, Loss lass);

/*----------------------------------------------------------------------------//
* MAIN
*-----------------------------------------------------------------------------*/

int main(){

    // defines output
    std::ofstream output("FDTD.dat", std::ofstream::out);

    int final_time = 3000;
    double eps = 377.0;

    // define initial E and H fields
    // std::vector<double> Ez(space, 0.0), Hy(space, 0.0);
    Field EM;
    EM.t = 0;

    FDTD(EM, final_time, eps, space, output);

}

/*----------------------------------------------------------------------------//
* SUBROUTINES
*-----------------------------------------------------------------------------*/

// This is the function we writs the bulk of the code in
void FDTD(Field EM,
          const int final_time, const double eps, const int space,
          std::ofstream& output){

    double loss = 0.00;
    double Cour = 1 / sqrt(2);

    Loss lass;
    lass  = createloss2d(lass, eps, Cour, loss);
    Loss1d lass1d;
    lass1d = createloss1d(lass1d, eps, Cour, loss);

    // Time looping
    for (int t = 0; t < final_time; t++){

        EM = Hupdate2d(EM, lass, t);
        EM = TFSF(EM, lass, lass1d, Cour);
        EM = Eupdate2d(EM,lass,t);
        EM = ABCcheck(EM, lass);
        // EM.Ez(0,100) = ricker(t, 0, Cour);
        
        // Outputting to a file
        int check = 5;
        if (t % check == 0){
            for (int dx = 0; dx < space; dx = dx + check){
                for (int dy = 0; dy < space; dy = dy + check){
                    output << t << '\t' << dx <<'\t' << dy << '\t'
                           << EM.Ez(dx, dy) << '\t' << EM.Hy(dx, dy) 
                           << '\t' << EM.Hx(dx, dy) << '\t' << '\n';
                }
            }

            output << '\n' << '\n';
        }

    }
}

// Adding the ricker solution
double ricker(int time, int loc, double Cour){
    double Ricky;
    double temp_const = 3.14159*((Cour*(double)time - (double)loc)/20.0 - 1.0);
    temp_const = temp_const * temp_const;
    Ricky = (1.0 - 2.0 * temp_const) * exp(-temp_const);
    return Ricky;

}

// 2 dimensional functions for E / H movement
Field Hupdate2d(Field EM, Loss lass, int t){
    // update magnetic field, x direction
    for (size_t dx = 0; dx < space; dx++){
        for (size_t dy = 0; dy < space - 1; dy++){
           EM.Hx(dx,dy) = lass.HxH(dx,dy) * EM.Hx(dx, dy) 
                       - lass.HxE(dx,dy) * (EM.Ez(dx,dy + 1) 
                                            - EM.Ez(dx,dy));
        }
    }


    // update magnetic field, y direction
    for (size_t dx = 0; dx < space - 1; dx++){
        for (size_t dy = 0; dy < space; dy++){
           EM.Hy(dx,dy) = lass.HyH(dx,dy) * EM.Hy(dx,dy) 
                      + lass.HyE(dx,dy) * (EM.Ez(dx + 1,dy) 
                                            - EM.Ez(dx,dy));
        }
    }

    return EM;

}


Field Eupdate2d(Field EM, Loss lass, int t){
    // update electric field
    for (size_t dx = 1; dx < space - 1; dx++){
        for (size_t dy = 1; dy < space - 1; dy++){
           EM.Ez(dx,dy) = lass.EzE(dx,dy) * EM.Ez(dx,dy)
                       + lass.EzH(dx,dy) * ((EM.Hy(dx, dy)
                                         - EM.Hy(dx - 1, dy))
                                         - (EM.Hx(dx,dy)
                                         - EM.Hx(dx, dy - 1)));
        }
    }
    return EM;
}

// 1 dimensional update functions for E / H
Field Hupdate1d(Field EM, Loss1d lass1d, int t){
    // update magnetic field, y direction
    for (size_t dx = 1; dx < space - 1; dx++){
        EM.Hy1d[dx] = lass1d.HyH[dx] * EM.Hy1d[dx] 
                  + lass1d.HyE[dx] * (EM.Ez1d[dx + 1] - EM.Ez1d[dx]);
    }

    return EM;
}

Field Eupdate1d(Field EM, Loss1d lass1d, int t){
    // update electric field, y direction
    for (size_t dx = 1; dx < space - 1; dx++){
        EM.Ez1d[dx] = lass1d.EzE[dx] * EM.Ez1d[dx] 
                  + lass1d.EzH[dx] * (EM.Hy1d[dx] - EM.Hy1d[dx - 1]);
    }

    return EM;

}

// Creating loss
Loss createloss2d(Loss lass, double eps, double Cour, double loss){

    double radius = 40;
    int sourcex = 150;
    int sourcey = 100;
    double dist, var, Q, epsp, mup;
    for (size_t dx = 0; dx < space; dx++){
        for (size_t dy = 0; dy < space; dy++){
             dist = sqrt((dx - sourcex)*(dx - sourcex) 
                       + (dy - sourcey)*(dy - sourcey)); 
            // if (dx > 100 && dx < 150 && dy > 75 && dy < 125){
            if (dist < radius){
                // Q = cbrt(-(radius / dist) + sqrt((radius/dist) 
                //                                * (radius/dist) + (1.0/27.0)));
                // var = (Q - (1.0 / (3.0 * Q))) * (Q - (1.0/ (3.0 * Q)));
                var = 2;
                if (abs(var) > 1000){
                    var = 1000;
                }

                if (isnan(var)){
                    var = 1000;
                }
                
                epsp = eps / (var * var);
                mup = 1 / (var * var);
/*
                lass.EzH(dx, dy) = Cour * eps;
                lass.EzE(dx, dy) = 1.0;
                lass.HyH(dx, dy) = 1.0;
                lass.HyE(dx, dy) = Cour / eps;
                lass.HxE(dx, dy) = Cour / eps;
                lass.HxH(dx, dy) = 1.0;

                lass.EzH(dx, dy) =  Cour * epsp /(1.0 - loss);
                lass.EzE(dx, dy) = (1.0 - loss) / (1.0 + loss);
                lass.HyH(dx, dy) = (1.0 - loss) / (1.0 + loss);
                lass.HyE(dx, dy) = Cour * (mup / eps) / (1.0 + loss);
                lass.HxE(dx, dy) = Cour * (mup / eps) / (1.0 + loss);
                lass.HxH(dx, dy) = (1.0 - loss) / (1.0 + loss);
*/
                lass.EzH(dx, dy) = 0;
                lass.EzE(dx, dy) = 0;
                lass.HyH(dx, dy) = 0;
                lass.HyE(dx, dy) = 0;
                lass.HxE(dx, dy) = 0;
                lass.HxH(dx, dy) = 0;

            }
            else{
/*
                lass.EzH(dx, dy) =  Cour * eps;
                lass.EzE(dx, dy) = 1.0;
                lass.HyH(dx, dy) = 1.0;
                lass.HyE(dx, dy) = Cour / eps;
                lass.HxE(dx, dy) = Cour / eps;
                lass.HxH(dx, dy) = 1.0;
                
                lass.EzH(dx, dy) =  Cour * eps /(1.0 - loss);
                lass.EzE(dx, dy) = (1.0 - loss) / (1.0 + loss);
                lass.HyH(dx, dy) = (1.0 - loss) / (1.0 + loss);
                lass.HyE(dx, dy) = Cour * (1.0 / eps) / (1.0 + loss);
                lass.HxE(dx, dy) = Cour * (1.0 / eps) / (1.0 + loss);
                lass.HxH(dx, dy) = (1.0 - loss) / (1.0 + loss);

*/
                lass.EzH(dx, dy) = Cour * eps;
                lass.EzE(dx, dy) = 1.0;
                lass.HyH(dx, dy) = 1.0;
                lass.HyE(dx, dy) = Cour * (1.0 / eps);
                lass.HxE(dx, dy) = Cour * (1.0 / eps);
                lass.HxH(dx, dy) = 1.0;
                
            }
        }
    }


    return lass;
}
Loss1d createloss1d(Loss1d lass1d, double eps, double Cour, double loss){
    for (size_t dx = 0; dx < space; dx++){

        lass1d.EzH[dx] = Cour * eps;
        lass1d.EzE[dx] = 1.0;
        lass1d.HyH[dx] = 1.0;
        lass1d.HyE[dx] = Cour / eps;

    }


    return lass1d;

}

// TFSF boundaries
Field TFSF(Field EM, Loss lass, Loss1d lass1d, double Cour){

    int dx, dy;

    // TFSF boundary
    Bound first, last;
    first.x = 10; last.x = 290;
    first.y = 10; last.y = 290;

    // Update along right edge!
    dx = last.x;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Hy(dx,dy) += lass.HyE(dx, dy) * EM.Ez1d[dx];
    }

    // Updating along left edge
    dx = first.x - 1;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Hy(dx,dy) -= lass.HyE(dx, dy) * EM.Ez1d[dx+1];
    }

    // Updating along top
    dy = last.y;
    for (int dx = first.x; dx <= last.x; dx++){
        EM.Hx(dx,dy) -= lass.HxE(dx, dy) * EM.Ez1d[dx];
    }

    // Update along bot
    dy = first.y - 1;
    for (int dx = first.x; dx <= last.x; dx++){
        EM.Hx(dx,dy) += lass.HxE(dx, dy) * EM.Ez1d[dx];
    }

    // Insert 1d grid stuff here. Update magnetic and electric field
    Hupdate1d(EM, lass1d, EM.t);
    Eupdate1d(EM, lass1d, EM.t);
    EM.Ez1d[10] = ricker(EM.t,0, Cour);
    // EM.Ez1d[10] = planewave(EM.t, 15, Cour, 30, 40);
    EM.t++;
    std::cout << EM.t << '\n';

    // Check mag instead of ricker.
    // Update along right
    dx = last.x;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Ez(dx, dy) += lass.EzH(dx, dy) * EM.Hy1d[dx];
    }

    // Updating Ez along left
    dx = first.x;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Ez(dx, dy) -= lass.EzH(dx, dy) * EM.Hy1d[dx - 1];
    }

    return EM;

}

// Checking Absorbing Boundary Conditions (ABC)
Field ABCcheck(Field EM, Loss lass){

    // defining constant for  ABC
    double c1, c2, c3, temp1, temp2;
    temp1 = sqrt(lass.EzH(0,0) * lass.HyE(0,0));
    temp2 = 1.0 / temp1 + 2.0 + temp1;
    c1 = -(1.0 / temp1 - 2.0 + temp1) / temp2;
    c2 = -2.0 * (temp1 - 1.0 / temp1) / temp2;
    c3 = 4.0 * (temp1 + 1.0 / temp1) / temp2;
    size_t dx, dy;

    // Setting ABC for top
    for (dx = 0; dx < space; dx++){
        EM.Ez(dx, space - 1) = c1 * (EM.Ez(dx, space - 3) + EM.Etop(0, 1, dx))
                      + c2 * (EM.Etop(0, 0, dx) + EM.Etop(2, 0 , dx)
                              -EM.Ez(dx,space - 2) -EM.Etop(1, 1, dx))
                      + c3 * EM.Etop(1, 0, dx) - EM.Etop(2, 1, dx); 

       // memorizing fields...
        for (dy = 0; dy < 3; dy++){
            EM.Etop(dy, 1, dx) = EM.Etop(dy, 0, dx);
            EM.Etop(dy, 0, dx) = EM.Ez(dx, space - 1 - dy);
        }
    }

    // Setting ABC for bottom
    for (dx = 0; dx < space; dx++){
        EM.Ez(dx,0) = c1 * (EM.Ez(dx, 2) + EM.Ebot(0, 1, dx))
                      + c2 * (EM.Ebot(0, 0, dx) + EM.Ebot(2, 0 , dx)
                              -EM.Ez(dx,1) -EM.Ebot(1, 1, dx))
                      + c3 * EM.Ebot(1, 0, dx) - EM.Ebot(2, 1, dx); 

        // memorizing fields...
        for (dy = 0; dy < 3; dy++){
            EM.Ebot(dy, 1, dx) = EM.Ebot(dy, 0, dx);
            EM.Ebot(dy, 0, dx) = EM.Ez(dx, dy);
        }
    }

    // ABC on right
    for (dy = 0; dy < space; dy++){
        EM.Ez(space - 1,dy) = c1 * (EM.Ez(space - 3,dy) + EM.Eright(0, 1, dy))
                      + c2 * (EM.Eright(0, 0, dy) + EM.Eright(2, 0 , dy)
                              -EM.Ez(space - 2,dy) -EM.Eright(1, 1, dy))
                      + c3 * EM.Eright(1, 0, dy) - EM.Eright(2, 1, dy); 

        // memorizing fields...
        for (dx = 0; dx < 3; dx++){
            EM.Eright(dx, 1, dy) = EM.Eright(dx, 0, dy);
            EM.Eright(dx, 0, dy) = EM.Ez(space - 1 - dx, dy);
        }
    }


    // Setting ABC for left side of grid. Woo!
    for (dy = 0; dy < space; dy++){
        EM.Ez(0,dy) = c1 * (EM.Ez(2,dy) + EM.Eleft(0, 1, dy))
                      + c2 * (EM.Eleft(0, 0, dy) + EM.Eleft(2, 0 , dy)
                              -EM.Ez(1,dy) -EM.Eleft(1, 1, dy))
                      + c3 * EM.Eleft(1, 0, dy) - EM.Eleft(2, 1, dy); 

        // memorizing fields...
        for (dx = 0; dx < 3; dx++){
            EM.Eleft(dx, 1, dy) = EM.Eleft(dx, 0, dy);
            EM.Eleft(dx, 0, dy) = EM.Ez(dx, dy);
        }
    }

    return EM;
}


// Adding plane wave
double planewave(int time, int loc, double Cour, int ppw, double radius){
    double plane;

    plane = sin((2 * M_PI / (double)ppw) * (Cour * (double)time -
                 (double)loc));
    //plane = sin((double)(time-loc) * 3.5 / radius);

    return plane;
}

