use <lib.scad>

module fighterPlatform() {
	translate([0,0,0.5]) box([18,36,1]);
}

module fighterRib() {
	difference() {
		hull() {
			translate([0,0,12]) box([6,1,1]);
			translate([0,0,9.5]) box([14.5,1,1]);
			translate([0,0,3]) box([17.9,1,6]);
		}
		scale([0.93,1.1,0.95]) translate([0,0,-0.01]) hull() {
			translate([0,0,12]) box([6,1,1]);
			translate([0,0,9.5]) box([14.5,1,1]);
			translate([0,0,3]) box([17.9,1,6]);
		}
	}
}

module fighterRibs() {
	for (i = [0:1:3]) translate([0,-i*5.815,0]) fighterRib();
}

module fighterTrolley() {
	translate([0,-9,1.5]) box([16,18,0.5]);
}

module fighterRail() {
	box([0.25,34,0.25]);
}

module fighterRails() {
	translate([-6,0,1.25]) fighterRail();
	translate([0,0,1.25]) fighterRail();
	translate([6,0,1.25]) fighterRail();
}

module fighterAxis() {
	rotate([90,0,0]) cyl(0.15,0.15,18,$fn=8);
}

module fighterAxes() {
	translate([-2.7,-9,11.5]) fighterAxis();
	translate([2.7,-9,11.5]) fighterAxis();
	translate([-6.5,-9,9.25]) fighterAxis();
	translate([6.5,-9,9.25]) fighterAxis();
	translate([-8,-9,5.6]) fighterAxis();
	translate([8,-9,5.6]) fighterAxis();
}

module fighterTool() {
	box([1.5,2.5,0.25]);
}

module fighterTools() {
	translate([2.65,-1,11.4]) rotate([0,16,0]) fighterTool();
	translate([-2.65,-1,11.4]) rotate([0,-16,0]) fighterTool();
	translate([6.45,-1,9.15]) rotate([0,47,0]) fighterTool();
	translate([-6.45,-1,9.15]) rotate([0,-47,0]) fighterTool();
	translate([7.9,-1,5.5]) rotate([0,80,0]) fighterTool();
	translate([-7.9,-1,5.5]) rotate([0,-80,0]) fighterTool();
}

module fighterYard() {
	fighterPlatform();
	fighterRibs();
	fighterTrolley();
	fighterRails();
	fighterAxes();
	fighterTools();
}

//fighterYard();
//fighterPlatform();
//fighterRibs();
//fighterTrolley();
//fighterRails();
//fighterAxes();
//fighterTool();


module fighterHull() {
	hull() {
		translate([0,5,0]) rotate([90,0,0]) cyl(0.6,0.6,0.1,$fn=6);
		translate([0,-4,0]) rotate([90,0,0]) cyl(1.5,1.5,4,$fn=6);
	}
}

module fighterNose() {
	translate([0,5.7,0]) hull() {
		translate([0,1,0]) box([0.6,1,0.1]);
		translate([0,-1,0]) rotate([-30,0,0]) box([1.5,0.1,1.5]);
	}
}

module fighterEngine() {
	difference() {
		hull() {
			translate([0,-1.9,0]) rotate([90,0,0]) cyl(0.8,0.8,0.1,$fn=24);
			translate([0,-2,0]) rotate([90,0,0]) cyl(0.7,0.7,0.1,$fn=24);
			translate([0,1.9,0]) rotate([90,0,0]) cyl(0.8,0.8,0.1,$fn=24);
			translate([0,2,0]) rotate([90,0,0]) cyl(0.7,0.7,0.1,$fn=24);
		}
		translate([0,-2.2,0]) rotate([90,0,0]) cyl(0.6,0.6,1,$fn=24);
		translate([0,2.2,0]) rotate([90,0,0]) cyl(0.6,0.6,1,$fn=24);
	}
}

module fighterEngines() {
	translate([2.5,-5,1]) fighterEngine();
	translate([-2.5,-5,1]) fighterEngine();
	translate([2.5,-5,-1]) fighterEngine();
	translate([-2.5,-5,-1]) fighterEngine();
}

module fighterPlume() {
	rotate([-90,0,0]) cyl(0, 0.6, 5, $fn=8);
}

module fighterPlumes() {
	translate([2.5,-9.5,1]) fighterPlume();
	translate([-2.5,-9.5,1]) fighterPlume();
	translate([2.5,-9.5,-1]) fighterPlume();
	translate([-2.5,-9.5,-1]) fighterPlume();
}

module fighterWing() {
	hull() {
		translate([7.5,0,0]) box([0.1,1,0.3]);
		translate([0,0.5,0]) box([0.1,3,0.5]);
	}
}

module fighterWings() {
	union() {
		translate([0,-5,0]) rotate([0,10,0]) fighterWing();
		translate([0,-5,0]) rotate([0,-10,0]) fighterWing();
		mirror([1,0,0]) translate([0,-5,0]) rotate([0,10,0]) fighterWing();
		mirror([1,0,0]) translate([0,-5,0]) rotate([0,-10,0]) fighterWing();
	}
}

module fighterGun() {
	rotate([90,0,0]) cyl(0.1,0.1,4,$fn=8);
}

module fighterGuns() {
	rotate([0,10,0]) translate([7.5,-3.5,0]) fighterGun();
	rotate([0,-10,0]) translate([7.5,-3.5,0]) fighterGun();
	mirror([1,0,0]) rotate([0,10,0]) translate([7.5,-3.5,0]) fighterGun();
	mirror([1,0,0]) rotate([0,-10,0]) translate([7.5,-3.5,0]) fighterGun();
}

module fighterCockpit() {
	translate([0,0,0.05]) intersection() {
		fighterHull();
		translate([0,-0.5,1.5]) box([3,3,3]);
	}
}

module fighter() {
	fighterHull();
	fighterNose();
	fighterWings();
	fighterEngines();
	fighterGuns();
	fighterCockpit();
	fighterPlumes();
}

fighter();
//fighterHull();
//fighterNose();
//fighterEngines();
//fighterWings();
//fighterGuns();
//fighterCockpit();
//fighterPlumes();
//translate([0,0,5]) fighter();

