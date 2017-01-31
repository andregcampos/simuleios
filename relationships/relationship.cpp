/*-------------relationship.cpp-----------------------------------------------//
*
* Purpose: to model becoming an ideal relationship by shape matching and 
*          modification using shape contexts. Info here:
*              https://en.wikipedia.org/wiki/Shape_context
*
*   Notes: Ideal relationship shape will be a heart (aww)
*          Align shapes with hungarian algorithm, then match_shape
*          
*-----------------------------------------------------------------------------*/

#include <iostream>
#include <random>
#include <stdlib.h>
#include "../visualization/cairo/cairo_vis.h"

// Function to create ideal relationship shape (heart)
std::vector<vec> create_ideal_shape(int res);

// Function to return a random blob
std::vector<vec> create_blob(int res);

// Function to visualize the addition of two blobs
std::vector<vec> add_blob(std::vector<vec> blob_1, std::vector<vec> blob_2);

// Shape matching functions, random sampling is unnecessary here

// Function to generate shape contexts
// In principle, the shape contexts should be a vector of vector<double>
std::vector<std::vector<double>> gen_shape_context(std::vector<vec> shape, 
                                                   int res);

// function to place all points in a grid related to the point's location
std::vector<double> bin(vec point, std::vector<vec> shape, int res);

// Function to align shapes
std::vector<vec> align_shapes(std::vector<std::vector<double>> context_1, 
                              std::vector<std::vector<double>> context_2);

// Function to compute the shape distance
std::vector<vec> gen_shape_dist(std::vector<vec> shape_1,
                                std::vector<vec> shape_2);
/*
std::vector<vec> gen_shape_dist(std::vector<double> context_1,
                                std::vector<double> context_2);
*/

// Function to modify the shape (relationship) to match the perfect shape
std::vector<vec> match_shape(std::vector<vec> shape_1, 
                             std::vector<vec> shape_2);

// Function to output shape_contexts
void print_shape_context(std::vector<std::vector<double>> shape_context);

// Function to use chi^2 test for a histogram distance measure
double hist_dist(std::vector<double> hist_1, std::vector<double> hist_2);


/*----------------------------------------------------------------------------//
* MAIN
*-----------------------------------------------------------------------------*/

int main(){

    // Initialize all visualization components
    int fps = 30;
    double res_x = 400;
    double res_y = 400;
    vec res = {res_x, res_y};
    color bg_clr = {0, 0, 0, 0};
    color line_clr = {1, 1, 1, 1};

    std::vector<frame> layers = init_layers(3, res, fps, bg_clr);

    std::vector<vec> heart = create_ideal_shape(10);
    std::vector<vec> blob = create_blob(10);

    for (int i = 0; i < heart.size(); i++){
        std::cout << blob[i].x << '\t' << heart[i].y << '\n';
    }

    int bin_res = 5;
    std::vector<std::vector<double>> shape_context = 
        gen_shape_context(heart, bin_res);

    print_shape_context(shape_context);

    draw_array(layers[1], blob, 400, 400, line_clr);

    draw_layers(layers);

    for (int i = 0; i < layers.size(); i++){
        layers[i].destroy_all();
    }

}

/*----------------------------------------------------------------------------//
* SUBROUTINES
*-----------------------------------------------------------------------------*/

// Function to create ideal relationship shape (heart)
std::vector<vec> create_ideal_shape(int res){
    // Creating the vector to work with
    std::vector<vec> ideal_shape(res);

    double t = 0;
    for (int i = 0; i < res; i++){
        t = -M_PI + 2.0*M_PI*i/res;
        ideal_shape[i].x = 16 * sin(t)*sin(t)*sin(t) / 34.0 + 0.5;
        ideal_shape[i].y = (13*cos(t) - 5*cos(2*t) -2*cos(3*t)-cos(4*t))/34.0
                            +0.5;
    }

    return ideal_shape;
}


// Function to return a random blob
std::vector<vec> create_blob(int res){

    // Creating vector to work with
    std::vector<vec> blob(res);

    // Doing random things
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> variance(0,1);

    double t = 0;
    for (int i = 0; i < res; i++){
        t = 2.0*M_PI*i/res;
        //blob[i].x = cos(t)*(0.4+variance(gen)*0.1) + 0.5;
        //blob[i].y = sin(t)*(0.4+variance(gen)*0.1) + 0.5;
        blob[i].x = (15+variance(gen)) * sin(t)*sin(t)*sin(t) / 34.0 + 0.5;
        blob[i].y = ((12+variance(gen))*cos(t) 
                      -(4+variance(gen))*cos(2*t) 
                      -(1+variance(gen))*cos(3*t)
                      -(0.9 + variance(gen)*0.1)*cos(4*t))/34.0
                            +0.5;

    }

    return blob;
}

// Function to visualize the addition of two blobs
// Might change depending on the underlying blob shape (could be a heart)
std::vector<vec> add_blob(std::vector<vec> blob_1, std::vector<vec> blob_2){

    std::vector<vec> blob_sum = blob_1;

    // For now, we are taking whichever points of the two blobs is closest to
    // a circle
    double t;
    vec close_1, close_2;
    for (size_t i = 0; i < blob_1.size(); i++){
        t = 2.0*M_PI*i/blob_1.size();
        close_1.x = abs(blob_1[i].x - (cos(t)*0.5 + 0.5));
        close_1.y = abs(blob_1[i].y - (sin(t)*0.5 + 0.5));
        close_2.x = abs(blob_2[i].x - (cos(t)*0.5 + 0.5));
        close_2.y = abs(blob_2[i].y - (sin(t)*0.5 + 0.5));

        if (length(close_2) < length(close_1)){
            blob_sum[i] = blob_2[i];
        }
    }

    return blob_sum;

}

// Function to generate shape contexts
// In principle, the shape contexts should be a vector of vector<double>
std::vector<std::vector<double>> gen_shape_context(std::vector<vec> shape, 
                                                   int res){

    // Creating vector to work with
    std::vector<std::vector<double>> shape_context(shape.size());

    for (size_t i = 0; i < shape.size(); i++){
        shape_context[i] = bin(shape[i], shape, res);
    }

    return shape_context;
}

// function to place all points in a grid related to the point's location
// We can use any space we want for binning, try xy before log-polar
std::vector<double> bin(vec point, std::vector<vec> shape, int res){

    // Creating the vector to work with
    std::vector<double> bin_data(res*res);

    vec dist;
    vec bin_dim = {2.0 / res, 2.0 / res};

    // Bin position is the left / bottom -most corner
    vec bin_pos = {-1,-1};
    for (int i = 0; i < shape.size(); i++){

        // Resetting bin_pos to -1, -1
        bin_pos = {-1, -1};

        dist = point - shape[i];
        for (int j = 0; j < res; j++){
            for (int k = 0; k < res; k++){
                if (dist.x > bin_pos.x && dist.x < bin_pos.x + bin_dim.x &&
                    dist.y > bin_pos.y && dist.y < bin_pos.y + bin_dim.y){
                    bin_data[j + res*k] += 1.0;
                    std::cout << "check " << i << '\n';
                }
                bin_pos.y += bin_dim.y;
            }
            bin_pos.x += bin_dim.x;

            // Resetting bin_pos.y to -1 for loop
            bin_pos.y = -1;
        }
    }

    return bin_data;
}

// Function to output shape_contexts
void print_shape_context(std::vector<std::vector<double>> shape_context){

    int res = sqrt(shape_context[0].size());
    int index = 0;
    for (size_t i = 0; i < shape_context.size(); i++){
        for (int j = 0; j < res; j++){
            for (int k = 0; k < res; k++){
                index = k + j*res;
                std::cout << shape_context[i][index] << ' ';
            }
            std::cout << '\n';
        }

        std::cout << '\n';
    }

}

// Function to compute the shape distance
std::vector<vec> gen_shape_dist(std::vector<vec> shape_1,
                                std::vector<vec> shape_2){

    // Creating vector to work with
    std::vector<vec> shape_dist(shape_1.size());

    for(size_t i = 0; i < shape_dist.size(); i++){
        shape_dist[i] = shape_1[i] - shape_2[i];
    }

    return shape_dist;
}

// Function to use chi^2 test for a histogram distance measure
double hist_dist(std::vector<double> hist_1, std::vector<double> hist_2){

    double distance = 0;

    for (size_t i = 0; i < hist_1.size(); i++){
        if (hist_1[i] + hist_2[i] != 0){
            distance += 0.5*pow(hist_1[i] - hist_2[i], 2) 
                             / (hist_1[i] + hist_2[i]);
        }
    }
    return distance;
}
