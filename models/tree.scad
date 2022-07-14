use <lib.scad>

function rand(s) = rands(0,1000,1,s)[0]/1000;

module branchHD(s) {
	polyhedron(points = [
		[0.8 + rand(s*2)*0.5,1,0],
		[0,0.8,0],
		[-0.8 - rand(s*3)*0.5,1,0],
		[0,-1.6,0],
		[0,-1.6,-0.5],
	],
	faces = [
		[0,1,2,3],
		[3,4,2],
		[4,2,1],
		[4,1,0],
		[4,3,0],
	]);
}

//branchHD(1);

module layerHD() {
	for (i = [0:60:300]) rotate([0,0,i]) rotate([-30,0,0]) translate([0,1.5,0]) branchHD(i);
}

function offset(i) = [20,-20,15,-15,10,-10][i];

module canopyHD() {
	union() {
		translate([0,0,1.5]) for (i = [0:1:5])
			translate([0,0,i*0.84]) rotate([0,0,offset(i)]) scale([1-(i*0.1),1-(i*0.1),1]) layerHD();
		translate([0,0,5.5]) cyl(0.8,0,1,$fn=6);
		translate([0,0,2.5]) cyl(0.3,0.3,5,$fn=6);
	}
}

module branchLD(s) {
	polyhedron(points = [
		[0.8 + rand(s*2)*0.5,1,0],
		[-0.8 - rand(s*3)*0.5,1,0],
		[0,-1.6,0],
		[0,-1.6,-0.5],
	],
	faces = [
		[0,1,2],
		[3,1,2],
		[3,1,0],
		[3,2,0],
	]);
}

//branchLD(1);

module layerLD() {
	for (i = [0:60:300]) rotate([0,0,i]) rotate([-30,0,0]) translate([0,1.5,0]) branchLD(i);
}

module canopyLD() {
	union() {
		translate([0,0,1.5]) for (i = [0:1:5])
			translate([0,0,i*0.84]) rotate([0,0,offset(i)]) scale([1-(i*0.1),1-(i*0.1),1]) layerLD();
		translate([0,0,5.5]) cyl(0.8,0,1,$fn=6);
		translate([0,0,2.5]) box([0.5,0.5,5]);
	}
}

module branchLD2() {
	translate([0,0,-0.6]) cyl(2.5,0,1.25,$fn=6);
}

module layerLD2() {
	branchLD2();
}

module canopyLD2() {
	union() {
		translate([0,0,1.5]) for (i = [0:1:5])
			translate([0,0,i*0.84]) rotate([0,0,offset(i)]) scale([1-(i*0.1),1-(i*0.1),1]) layerLD2();
		translate([0,0,5.5]) cyl(0.8,0,1,$fn=6);
	}
}

module branchVLD() {
	translate([0,0,-0.6]) cyl(3,0,1.25,$fn=4);
}

module layerVLD() {
	branchVLD();
}

module canopyVLD() {
	union() {
		translate([0,0,1.5]) for (i = [0:1:5])
			translate([0,0,i*0.84]) rotate([0,0,offset(i)]) scale([1-(i*0.1),1-(i*0.1),1]) layerVLD();
	}
}

//translate([-6,0,0])
//	canopyHD();
//translate([0,0,0])
	canopyLD();
//translate([6,0,0])
//	canopyLD2();
//translate([12,0,0])
//	canopyVLD();
