use <lib.scad>

module baseHD() {
	difference() {
		minkowskiOutsideRound(10,$fn=8)
		translate([0,0,1000-50]) box([1000, 1000, 150]);
		translate([-225,0,1000]) box([425, 1100, 100]);
		translate([225,0,1000]) box([425, 1100, 100]);
	}
}

module baseLD() {
	difference() {
		translate([0,0,1000-50]) box([1000, 1000, 150]);
		translate([-225,0,1000]) box([425, 1100, 100]);
		translate([225,0,1000]) box([425, 1100, 100]);
	}
}

module pillar() {
	translate([0,0,450]) box([500, 500, 900]);
}

module belt() {
	translate([0,0,995]) box([850, 1000, 10]);
}

module beltVLD() {
	translate([0,0,995]) box([850, 1000, 50]);
}

module ridge() {
	translate([0,0,1.005]) union() {
		box([0.800, 0.030, 0.010]);
		translate([-0.21,-0.03,0]) rotate([0,0,30]) cyl(0.075,0.075,0.010,$fn=3);
		translate([0.21,-0.03,0]) rotate([0,0,30]) cyl(0.075,0.075,0.010,$fn=3);
	}
}

module baseRightHD($fn=64) {
	translate([0,0,1000-50]) union() {
		translate([500,-500,0]) intersection() {
			cyl(50, 50, 100);
			#translate([-25,25,0]) box([50, 50, 100]);
		}
		translate([500,-500,0]) difference() {
			minkowskiOutsideRound(10,$fn=8)
			intersection() {
				cyl(1000, 1000, 150,$fn=64);
				translate([-500,500,0]) box([1000, 1000, 200]);
			}
			translate([0,0,50]) difference() {
				cyl(937, 937, 100);
				cyl(513, 513, 110);
			}
			translate([0,0,50]) difference() {
				cyl(487, 487, 100);
				cyl(63, 63, 110);
			}
		}
	}
}

module baseRightLD($fn=24) {
	translate([0,0,1000-50]) union() {
		translate([500,-500,0]) intersection() {
			cyl(50, 50, 100);
			translate([-25,25,0]) box([50, 50, 100]);
		}
		translate([500,-500,0]) difference() {
//			minkowskiOutsideRound(10,$fn=8)
			intersection() {
				cyl(1000, 1000, 150,$fn=24);
				translate([-500,500,0]) box([1000, 1000, 200]);
			}
			translate([0,0,50]) difference() {
				cyl(937, 937, 100);
				cyl(513, 513, 110);
			}
			translate([0,0,50]) difference() {
				cyl(487, 487, 100);
				cyl(63, 63, 110);
			}
		}
	}
}

module baseLeftHD() {
	mirror([1,0,0]) baseRightHD();
}

module baseLeftLD() {
	mirror([1,0,0]) baseRightLD();
}

module beltRight(s=1) {
	translate([0,0,995]) scale([1,1,s]) intersection() {
		translate([500,-500,0]) difference() {
			cyl(925, 925, 10);
			cyl( 75,  75, 20);
		}
		box([1000, 1000, 200]);
	}
}

module beltLeft(s=1) {
	mirror([1,0,0]) beltRight(s);
}

module unveyor() {
	translate([0,0,1000]) difference() {
		box([1000,2000,1500]);
		translate([0,501,0]) box([900,1001,1400]);
		translate([0,-1800,0]) rotate([-36.5,0,0]) box([2000,2000,5000]);
		translate([0,2000,0]) rotate([10,0,0]) box([2000,2000,2000]);
	}
}

module loader() {
	translate([0,0,750]) difference() {
		box([1000,2000,1500]);
		translate([0,501,0]) box([900,1001,1400]);
		translate([0,2000,0]) rotate([10,0,0]) box([2000,2000,2000]);
	}
}

module balancer() {
	difference() {
		translate([0,0,1200]) box([1000, 1000, 600]);
		translate([0,0,1200]) box([900, 1100, 500]);
	}
}

//baseHD();
//baseLD();

//fillet(10,$fn=8) loader();
//loader();

//fillet(10,$fn=8) pillar();
//pillar();

//beltVLD();
ridge();

//baseRightHD();
//baseRightLD();
//baseLeftHD();
//baseLeftLD();

//beltLeft($fn=64);
//beltLeft($fn=24);

//beltRight($fn=64);
//beltRight($fn=24);

//fillet(10,$fn=8) unveyor();
//unveyor();

//fillet(10,$fn=8) balancer();
//balancer();
