use <lib.scad>;

//seed = floor(rands(1,1000,1)[0]);
//seed = 268;
//seed = 431;
seed = 84;
echo(seed);

// Module randomRock : draws a random rock
module randomRock(radius,corner_size,number_of_corners) {
  // Draw rock
  hull(){
    // Make a hull..
    for(i=[0:number_of_corners]){
      // random position in 'sphere'
      y = rands(0,360,1,seed*i+1)[0];
      z = rands(0,360,1,seed*i+2)[0];
      rotate([0,y,z])

      // translate radius (minus half of corner size)
      translate([radius-corner_size*0.5,0,0])

      // draw cube as corner
      rotate([rands(0,360,1,seed*i+3)[0],rands(0,360,1,seed*i+4)[0],rands(0,360,1,seed*i+5)[0]]) // random rotation
      cube(rands(corner_size*0.5,corner_size,1,seed*i+6)[0],center = true);
    }
  }
}

// : More corners --> More 'spheric' central stone
number_of_corners = rands(15,25,1,seed+7)[0]; // [5:30]

// : Deforms central stone in vertical
stone_scale = 1; // [0.2:0.1:3]

// Number of medium size stones
medium_stones = rands(3,8,1,seed+8)[0]; // [5:30]

// Number of small stones
//small_stones = 0; //rands(3,8,1,seed+9)[0]; // [5:30]

/* [Hidden] */
radius = 20;
corner_size = 2;

module hd() {
	scale([0.05,0.05,0.05])
		translate([0,0,-10]) intersection() {
		  union(){

		    // big central stone
		    scale([1,1,stone_scale])
			    randomRock(radius,corner_size,number_of_corners);

		    // medium size stones
		    if(medium_stones>0){
		      for(i=[1:medium_stones]){
		        // random position in 'circle'
		        z = rands(0,360,1,seed*i+10)[0];
		        rotate([0,0,z])

		        // translate radius
		        translate([rands(radius*0.6,radius*0.9,1,seed*i+11)[0],0,0])
		        randomRock(rands(radius*0.3,radius*0.5,1,seed*i+12)[0],corner_size*0.5,15);
		      }
		    }
		  }
		  // cut half of figure
		  translate([-50,-50,0]) cube(100);
	}
}

module ld() {
	scale([0.05,0.05,0.05])
		translate([0,0,-10]) intersection() {
		  union() {

		    scale([1,1,stone_scale])
			    sphere(radius*0.9,$fn=5);

		    if(medium_stones > 0) {
		      for(i=[1:medium_stones]){
		        // random position in 'circle'
		        z = rands(0,360,1,seed*i+10)[0];
		        rotate([0,0,z])

		        // translate radius
		        translate([rands(radius*0.6,radius*0.9,1,seed*i+11)[0],0,0])
		        sphere(rands(radius*0.3,radius*0.5,1,seed*i+12)[0]*0.8,$fn=5);
		      }
		    }
		  }
		  // cut half of figure
		  translate([-50,-50,0]) cube(100);
		}
}

module vld() {
	scale([0.05,0.05,0.05])
		translate([0,0,-10]) intersection() {
		  union() {

		    scale([1,1,stone_scale])
			    sphere(radius*0.9,$fn=5);

//		    if(medium_stones > 0) {
//		      for(i=[1:medium_stones]){
//		        // random position in 'circle'
//		        z = rands(0,360,1,seed*i+10)[0];
//		        rotate([0,0,z])
//
//		        // translate radius
//		        translate([rands(radius*0.6,radius*0.9,1,seed*i+11)[0],0,0])
//		        sphere(rands(radius*0.3,radius*0.5,1,seed*i+12)[0]*0.8,$fn=5);
//		      }
//		    }
		  }
		  // cut half of figure
		  translate([-50,-50,0]) cube(100);
		}
}

//hd();
//ld();
vld();