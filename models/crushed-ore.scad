use <lib.scad>;

function rand(a,b) = rands(a,b,1)[0];
function randDegree() = rand(0,360);

corners = 20;
radius = 0.4;
corner_size = 0.1;

module randomRock(radius, corner_size) {
  hull() {
    for (i = [0:corners]) {
      y = randDegree();
      z = randDegree();
      r = rand(corner_size*0.5,corner_size);

      rotate([0,y,z])
      translate([radius-corner_size*0.5,0,0])
      rotate([randDegree(),randDegree(),randDegree()])
      cylinder(r1=r, r2=0, h=r, $fn=3);
    }
  }
}

intersection() {
	union() {
		for (i = [-0.3:0.1:0.3]) {
			for (j = [-0.3:0.1:0.3]) {
				translate([i,j,-radius/1.5])
					randomRock(rand(radius,radius),corner_size*0.5);
			}
		}
		box([1, 1, 0.15]);
	}
	//translate([0,0,0.5]) rbox([0.6, 0.6, 1], 0.05, $fn=12);
	translate([0,0,0.5]) box([0.6, 0.6, 1]);
}
