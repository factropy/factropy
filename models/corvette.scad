use <lib.scad>

edge=1;

module weaponCutout() {
	module forward() {
		module cutout() {
			hull() {
				box([3,3,3]);
				translate([6,0,1.5]) box([6,6,6]);
			}
		}
		translate([2.6,-10,1.2]) rotate([0,0,0]) cutout();
		translate([-2.6,-10,1.2]) rotate([0,0,180]) cutout();
	}
	module rear() {
		module cutoutAbove() {
			hull() {
				box([3,6,3]);
				translate([6,0,1.5]) box([6,10,6]);
			}
		}
		module cutoutBelow() {
			hull() {
				box([3,6,3]);
				translate([6,0,-1.5]) box([6,10,6]);
			}
		}

		union() {
			translate([7,9,1.2]) rotate([0,0,0]) cutoutAbove();
			translate([-7,9,1.2]) rotate([0,0,180]) cutoutAbove();
			translate([7,9,-2.1]) rotate([0,0,0]) cutoutBelow();
			translate([-7,9,-2.1]) rotate([0,0,180]) cutoutBelow();
		}
	}
	union() {
		forward();
		rear();
	}
}

module weaponMounts() {
	module small() {
		module barrel() {
			rotate([0,-20,0]) translate([0.6,0,0.4]) rotate([0,90,0]) cyl(0.1,0.1,2.8,$fn=8);
		}
		union() {
			intersection() {
				union() {
					translate([0,0,0.4]) sphere(1,$fn=32);
					translate([0,0,-0.5]) cyl(1.1,1.1,1,$fn=32);
				}
				translate([0,0,1.21]) box([3,3,3]);
			}
			translate([0,0.15,0.15]) barrel();
			translate([0,-0.15,0.15]) barrel();
			translate([0,0.15,-0.15]) barrel();
			translate([0,-0.15,-0.15]) barrel();
		}
	}
	module large() {
		module barrel() {
			translate([0,-2,0.6]) rotate([90,0,0]) cyl(0.2,0.2,6,$fn=16);
		}
		union() {
			intersection() {
				union() {
					translate([0,0,0.4]) sphere(1.5,$fn=32);
					translate([0,0,-0.5]) cyl(1.6,1.6,1,$fn=32);
				}
				translate([0,0,1.21]) box([4,4,3]);
			}
			translate([0.3,0,0.3]) barrel();
			translate([-0.3,0,0.3]) barrel();
			translate([0.3,0,-0.2]) barrel();
			translate([-0.3,0,-0.2]) barrel();
		}
	}
	translate([2.2,-10,0]) small();
	translate([-2.2,-10,0]) rotate([0,0,180]) small();

	translate([7.2,11,0]) large();
	translate([-7.2,11,0]) large();
}

module edgeCutout() {
	difference() {
		hull() {
			translate([0,-20.1,0]) box([2,1,edge*0.8]);
			translate([9.1,14.1,0]) box([1,3,edge*0.8]);
			translate([-9.1,14.1,0]) box([1,3,edge*0.8]);
		}
		hull() {
			translate([0,-19.6,0]) box([1.5,1,edge]);
			translate([8.6,14,0]) box([1,3,edge]);
			translate([-8.6,14,0]) box([1,3,edge]);
		}
		translate([0,15.5,0]) box([18.2,1,1]);
	}
}

module edgeInset() {
	difference() {
		hull() {
			translate([0,-19.8,0]) box([1.7,1,edge]);
			translate([8.8,14,0]) box([1,3,edge]);
			translate([-8.8,14,0]) box([1,3,edge]);
		}
		hull() {
			translate([0,-19.6,0]) box([1.5,1,edge*1.1]);
			translate([8.6,14,0]) box([1,3,edge*1.1]);
			translate([-8.6,14,0]) box([1,3,edge*1.1]);
		}
		translate([0,15.5,0]) box([18.2,1,1]);
		weaponCutout();
	}
}

module drive() {
	module thruster() {
		module bell(s) {
			hull() for (i = [0:0.1:1])
				rotate([0,0,180]) scale(s) translate([0,i,0]) rotate([90,0,0]) cyl(1-i*i, 1-i, 0.1,$fn=32);
		}
		difference() {
			bell([2,4,2]);
			bell([1.8,4.1,1.8]);
		}
	}
	translate([0,21,0]) union() {
		translate([5,0,0]) thruster();
		translate([0,0,0]) thruster();
		translate([-5,0,0]) thruster();
		translate([0,-4,0]) rbox([15,3,2,],0.1,$fn=8);
	}
}

module driveGlow() {
	translate([0,21,0]) union() {
		translate([5,0,0]) rotate([90,0,0]) cyl(1.9,1.9,0.1,$fn=32);
		translate([0,0,0]) rotate([90,0,0]) cyl(1.9,1.9,0.1,$fn=32);
		translate([-5,0,0]) rotate([90,0,0]) cyl(1.9,1.9,0.1,$fn=32);
	}
}

module drivePlumeOuter() {
	module plume() {
		module bell(s) {
			hull() {
				for (i = [0:0.1:1])
					scale(s) translate([0,i,0]) rotate([90,0,0]) cyl(1-i*i, 1-i, 0.1,$fn=16);
				translate([0,s[1]/1.5,0]) scale([1,s[1],1]) rotate([90,0,0]) cyl(0.01, 0.01, 1, $fn=8);
			}
		}
		difference() {
			bell([1.6,15,1.6]);
		}
	}
	translate([0,20,0]) union() {
		translate([5,0,0]) plume();
		translate([0,0,0]) plume();
		translate([-5,0,0]) plume();
	}
}

module drivePlumeInner() {
	module plume() {
		module bell(s) {
			hull() {
				for (i = [0:0.1:1])
					scale(s) translate([0,i,0]) rotate([90,0,0]) cyl(1-i*i, 1-i, 0.1,$fn=16);
				translate([0,s[1]/1.5,0]) scale([1,s[1],1]) rotate([90,0,0]) cyl(0.01, 0.01, 1, $fn=8);
			}
		}
		difference() {
			bell([1,12,1]);
		}
	}
	translate([0,20,0]) union() {
		translate([5,0,0]) plume();
		translate([0,0,0]) plume();
		translate([-5,0,0]) plume();
	}
}

module body() {
	difference() {
		hull() {
			translate([0,-20,0]) box([2,1,edge]);
			translate([9,14,0]) box([1,3,edge]);
			translate([-9,14,0]) box([1,3,edge]);
			translate([0,12,0]) rotate([0,30,0]) rotate([90,0,0]) cyl(4,4,10,$fn=3);
		}
		edgeCutout();
		weaponCutout();
	}
}

//body();
//edgeInset();
//drive();
//driveGlow();
//#drivePlumeOuter();
//drivePlumeInner();

weaponMounts();

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

//%translate([0,0,-20]) shipyard2();