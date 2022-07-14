use <lib.scad>

module barLD() {
	box([0.6,0.2,0.1]);
}

module stackLD() {
	translate([0,0.15,0.05]) barLD();
	translate([0,-0.15,0.05]) barLD();
	rotate([0,0,90]) translate([0,0.15,0.149]) barLD();
	rotate([0,0,90]) translate([0,-0.15,0.149]) barLD();
	translate([0,0.15,0.248]) barLD();
	translate([0,-0.15,0.248]) barLD();
}

module barHD() {
	rbox([0.6,0.2,0.1],0.01,$fn=6);
}

module stackHD() {
	translate([0,0.15,0.05]) barHD();
	translate([0,-0.15,0.05]) barHD();
	rotate([0,0,90]) translate([0,0.15,0.149]) barHD();
	rotate([0,0,90]) translate([0,-0.15,0.149]) barHD();
	translate([0,0.15,0.248]) barHD();
	translate([0,-0.15,0.248]) barHD();
}

stackHD();
//stackLD();
