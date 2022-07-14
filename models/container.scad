use <lib.scad>

small = [2.95, 6.95, 3];
large = [3.95, 10.95, 4];

module sideCutOutsHD(body) {
	union() {
		translate([-0.06,0,0]) rotate([0,0,30]) cyl(0.1, 0.1, z(body)-0.2, $fn=6);
		for (i = [1:y(body)*2.4])
			translate([0,0.2*i,0]) translate([-0.06,0,0]) rotate([0,0,30]) cyl(0.1, 0.1, z(body)-0.2, $fn=6);
		for (i = [1:y(body)*2.4])
			translate([0,-0.2*i,0]) translate([-0.06,0,0]) rotate([0,0,30]) cyl(0.1, 0.1, z(body)-0.2, $fn=6);
	}
}

module sideCutOutsLD(body) {
	union() {
		//translate([-0.06,0,0]) rotate([0,0,0]) cyl(0.1, 0.1, z(body)-0.2, $fn=4);
		for (i = [1:2:y(body)*2.4])
			translate([0,0.2*i,0]) translate([-0.06,0,0]) rotate([0,0,30]) cyl(0.15, 0.15, z(body)-0.4, $fn=6);
		for (i = [1:2:y(body)*2.4])
			translate([0,-0.2*i,0]) translate([-0.06,0,0]) rotate([0,0,30]) cyl(0.15, 0.15, z(body)-0.4, $fn=6);
	}
}

module sideCutOuts3HD(body) {
	rotate([0,000,0]) translate([-x(body)/2+0.02,0,0]) sideCutOutsHD(body);
	rotate([0,090,0]) translate([-x(body)/2,0,0]) sideCutOutsHD(body);
	rotate([0,180,0]) translate([-x(body)/2+0.02,0,0]) sideCutOutsHD(body);
}

module sideCutOuts3LD(body) {
	rotate([0,000,0]) translate([-x(body)/2+0.02,0,0]) sideCutOutsLD(body);
	rotate([0,090,0]) translate([-x(body)/2,0,0]) sideCutOutsLD(body);
	rotate([0,180,0]) translate([-x(body)/2+0.02,0,0]) sideCutOutsLD(body);
}

module doorFrame(body) {
	box([x(body)/2-0.15,0.05,z(body)-0.2]);
}

module doorsHD(body) {
	translate([x(body)/4-0.025,0,0]) doorFrame(body);
	for (i = [-z(body)/2+0.3:0.3:z(body)/2-0.2])
		translate([x(body)/4-0.025,0,i]) rotate([0,90,0]) cyl(0.1,0.1,x(body)/2-0.3,$fn=6);
	translate([-x(body)/4+0.025,0,0]) doorFrame(body);
	for (i = [-z(body)/2+0.3:0.3:z(body)/2-0.2])
		translate([-x(body)/4+0.025,0,i]) rotate([0,90,0]) cyl(0.1,0.1,x(body)/2-0.3,$fn=6);
}

module doors2HD(body) {
	translate([0,y(body)/2,0]) doorsHD(body);
	translate([0,-y(body)/2,0]) rotate([0,0,180]) doorsHD(body);
}

module doorsLD(body) {
	translate([x(body)/4-0.025,0,0]) doorFrame(body);
	for (i = [-z(body)/2+0.6:0.6:z(body)/2-0.4])
		translate([x(body)/4-0.025,0.05,i]) rotate([0,90,0]) cyl(0.2,0.2,x(body)/2-0.3,$fn=6);
	translate([-x(body)/4+0.025,0,0]) doorFrame(body);
	for (i = [-z(body)/2+0.6:0.6:z(body)/2-0.4])
		translate([-x(body)/4+0.025,0.05,i]) rotate([0,90,0]) cyl(0.2,0.2,x(body)/2-0.3,$fn=6);
}

module doors2LD(body) {
	translate([0,y(body)/2,0]) doorsLD(body);
	translate([0,-y(body)/2,0]) rotate([0,0,180]) doorsLD(body);
}

module hd(body) {
	difference() {
		rbox(body,0.01,$fn=8);
		sideCutOuts3HD(body);
		doors2HD(body);
	}
}

module ld(body) {
	difference() {
		box(body);
		sideCutOuts3LD(body);
		doors2LD(body);
	}
}

//ld(small);
//hd(large);
hull() ld(large);