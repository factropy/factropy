use <lib.scad>

module wheelHD() {
	rotate([90,0,0]) union() {
		minkowskiOutsideRound(0.05,$fn=8)
		difference() {
			cyl(5,5,2,$fn=64);
			cyl(4.5,4.5,3,$fn=64);
		}
		difference() {
			hull() {
				cyl(5,5,0.5,$fn=64);
				cyl(1,1,2,$fn=64);
			}
			for (i = [0:45:315])
				#rotate([0,0,i]) translate([0,2.7,0.7]) rotate([-10.5,0,0]) box([0.1,2,0.1]);
			for (i = [0:45:315])
				#rotate([0,0,i]) translate([0,2.7,-0.7]) rotate([10.5,0,0]) box([0.1,2,0.1]);
		}
	}
}

module wheelLD() {
	rotate([90,0,0]) union() {
		difference() {
			cyl(5,5,2,$fn=32);
			cyl(4.5,4.5,3,$fn=32);
		}
		difference() {
			hull() {
				cyl(5,5,0.5,$fn=32);
				cyl(1,1,2,$fn=32);
			}
		}
	}
}

module baseHD() {
	//minkowskiOutsideRound(0.02,$fn=8)
	difference() {
		hull() {
			box([11.975,3.975,1]);
			translate([0,0,0.5]) rotate([90,0,0]) cyl(1,1,4,$fn=32);
		}
		translate([0,0,0.25]) box([10,3,5]);
	}
}

module baseLD() {
	difference() {
		hull() {
			box([11.975,3.975,1]);
			translate([0,0,0.5]) rotate([90,0,0]) cyl(1,1,4,$fn=8);
		}
		translate([0,0,0.25]) box([10,3,5]);
	}
}

module axelHD() {
	//minkowskiOutsideRound(0.05,$fn=8)
	rotate([90,0,0]) cyl(0.5,0.5,4.1,$fn=32);
}

module axelLD($fn=8) {
	rotate([90,0,0]) cyl(0.5,0.5,4.1);
}

//translate([0,0,1]) wheelHD();
//translate([0,0,0.5]) baseHD();
//translate([0,0,1]) axelHD();

//wheelHD();
//spokeHD();
//baseHD();
//axelHD();

wheelLD();
//spokeLD();
//baseLD();
//axelLD();
