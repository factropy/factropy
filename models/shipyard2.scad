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

module shipyard2SubFrame() {
	translate([0,0,2]) rotate([90,0,0]) union() {
		difference() {
			cyl(18,18,1,$fn=32);
			cyl(17,17,2,$fn=32);
			translate([9,0,0]) box([20,50,20]);
			translate([0,-10,0]) box([50,20,20]);
		}
	}
}

module shipyard2Frame() {
	translate([]) union() {
		translate([0,15,0]) shipyard2SubFrame();
		translate([0,0,0]) shipyard2SubFrame();
		translate([0,-15,0]) shipyard2SubFrame();
		translate([-0.55,0,19.5]) box([1,34,1]);
	}
}

module shipyard2Frames() {
	rotate([0,0,0]) shipyard2Frame();
	rotate([0,0,180]) shipyard2Frame();
}

module shipyard2Lift() {
	hull() {
		translate([0,15,1.5]) cyl(12,12,1,$fn=8);
		translate([0,-15,1.5]) cyl(12,12,1,$fn=8);
	}
}

module shipyard2ScreenShort() {
	translate([0,0,8]) box([20,0.1,8]);
}

module shipyard2ScreenLong() {
	translate([0,0,8]) box([0.1,42,8]);
}

module shipyard2() {
	difference() {
		hull() {
			translate([0,0,1]) box([40,60,2]);
			translate([0,14,11]) cyl(13.5,13.5,1,$fn=8);
			translate([0,-14,11]) cyl(13.5,13.5,1,$fn=8);
		}
		hull() {
			translate([0,14,6]) cyl(12.5,12.5,11.5,$fn=8);
			translate([0,-14,6]) cyl(12.5,12.5,11.5,$fn=8);
		}
		translate([16.5,-15,26]) box([4,1.5,50]);
		translate([16.5,0,26]) box([4,1.5,50]);
		translate([16.5,15,26]) box([4,1.5,50]);
		translate([-16.5,-15,26]) box([4,1.5,50]);
		translate([-16.5,0,26]) box([4,1.5,50]);
		translate([-16.5,15,26]) box([4,1.5,50]);
	}
}

//shipyard2();
//shipyard2Lift();
//shipyard2Frames();
shipyard2ScreenShort();
//shipyard2ScreenLong();
