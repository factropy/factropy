use <lib.scad>

//module corvettePlatform() {
//	translate([0,0,0.5]) box([30,15,1]);
//}
//
//corvettePlatform();
//
//module destroyerPlatform() {
//	translate([0,0,0.5]) box([60,30,1]);
//}
//
//destoyerPlatform();

module shipyard1Frame() {
	translate([0,0,0]) rotate([90,0,0]) union() {
		difference() {
			cyl(18,18,1,$fn=32);
			cyl(17,17,2,$fn=32);
			translate([9,0,0]) box([20,50,20]);
			translate([0,-10,0]) box([50,20,20]);
		}
		translate([0,17.5,0]) rotate([0,45,0]) rotate([90,0,0]) difference() {
			cyl(4,4,1,$fn=8);
			translate([0,3.98,0]) box([8,8,8]);
			translate([3.98,0,0]) box([8,8,8]);
		}
	}
}

module shipyard1Frames() {
	rotate([0,0,45]) shipyard1Frame();
	rotate([0,0,-45]) shipyard1Frame();
	rotate([0,0,135]) shipyard1Frame();
	rotate([0,0,-135]) shipyard1Frame();
}

module shipyard1Lift() {
	translate([0,0,1.5]) cyl(12,12,1,$fn=8);
}

module shipyard1Screen() {
	translate([0,0,6]) box([16,0.1,6]);
}

module shipyard1() {
	difference() {
		hull() {
			translate([0,0,1]) box([30,30,2]);
			translate([0,0,9]) cyl(13,13,1,$fn=8);
		}
		translate([0,0,6]) cyl(12,12,10,$fn=8);
		rotate([0,0,45]) translate([16,0,8]) box([4,2,15]);
		rotate([0,0,135]) translate([16,0,8]) box([4,2,15]);
		rotate([0,0,-45]) translate([16,0,8]) box([4,2,15]);
		rotate([0,0,-135]) translate([16,0,8]) box([4,2,15]);
		translate([0,15,2]) box([2,4,2]);
	}
}

//shipyard1();
//shipyard1Lift();
//shipyard1Frames();
shipyard1Screen();