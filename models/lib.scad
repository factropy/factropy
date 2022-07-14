function x(v) = v[0];
function y(v) = v[1];
function z(v) = v[2];
function add(v1,v2) = [ for (i = [ 0 : len(v1)-1 ]) v1[i] + v2[i] ];
function xv(d) = [d,0,0];
function yv(d) = [0,d,0];
function zv(d) = [0,0,d];

module box(v) { cube(size=v, center=true); }
module cyl(r1,r2,h) { cylinder(r1=r1, r2=r2, h=h, center=true); }

function grow(n,v) = [x(v)+n*2, y(v)+n*2, z(v)+n*2];

module rbox(v, radius=1) {
	cx = x(v)/2-radius;
	cy = y(v)/2-radius;
	cz = z(v)/2-radius;

	hull() {
		translate([cx,cy,cz]) sphere(r=radius);
		translate([cx,cy,-cz]) sphere(r=radius);
		translate([cx,-cy,cz]) sphere(r=radius);
		translate([cx,-cy,-cz]) sphere(r=radius);
		translate([-cx,cy,cz]) sphere(r=radius);
		translate([-cx,cy,-cz]) sphere(r=radius);
		translate([-cx,-cy,cz]) sphere(r=radius);
		translate([-cx,-cy,-cz]) sphere(r=radius);
	}
}

module hull_chain() {
    union() {
        for (i = [0:$children-2]) {
            hull() {
                children(i);
                children(i+1);
            }
        }
    }
}

module pipe(points, r) {
    union() {
        for (i = [0:len(points)-2]) {
            a = points[i];
            b = points[i+1];
            hull() {
                translate(a) sphere(r);
                translate(b) sphere(r);
            }
        }
    }
}

// Library: MinkowskiRound.scad
// Version: 1.0
// Author: IrevDev
// Copyright: 2020
// License: MIT

/*
---Modules
round2d(outside radius,internal radius) -> takes 2d children
This module will round 2d any shape
minkowskiRound(outsideRadius,internalRadius,enable,envelopecube[x,y,z]) -> takes 3d children
This module will round any 3d shape though it takes a long time, possibly more than 12 hours (use a low $fn value, 10-15 to begin with) so make sure enable is set to 0 untill you are ready for final render,the envelopecube must be bigger than the object you are rounding, default values are [500,500,500].
minkowskiOutsideRound(radius,enable,envelopecube[x,y,z])
minkowskiInsideRound(radius,enable,envelopecube[x,y,z])
Both this modules do the same thing as minkowskiRound() but focus on either inside or outside radiuses, which means these modules use one less minkowski() (two instead of three) making them quicker if you don't want both inside and outside radiuses.
--Examples
*/

//round2d(1,6)difference(){square([20,20]);square([10,10]);}
//minkowskiRound(3,1.5,1)theshape();
//minkowskiInsideRound(3,1)theshape();
//minkowskiOutsideRound(3,1)theshape();

//---good test child for examples
//module theshape(){//example shape
//    difference(){
//        cube([20,20,20]);
//        cube([10,10,10]);
//    }
//}

// $fn=20;
// minkowskiRound(0.7,1.5,1,[50,50,50])union(){//--example in the thiniverse thumbnail/main image
//   cube([6,6,22]);
//   rotate([30,45,10])cylinder(h=22,d=10);
// }//--I rendered this out with a $fn=25 and it took more than 12 hours on my computer


module round2d(OR=3,IR=1){
  offset(OR){
    offset(-IR-OR){
      offset(IR){
        children();
      }
    }
  }
}

module minkowskiRound(OR=1,IR=1,boundingEnvelope=[10000,10000,10000]){
    minkowski(){//expand the now positive shape back out
      difference(){//make the negative shape positive again
        cube(boundingEnvelope-[0.1,0.1,0.1],center=true);
        minkowski(){//expand the negative shape inwards
          difference(){//create a negative of the children
            cube(boundingEnvelope,center=true);
            minkowski(){//expand the children
              children();
              sphere(IR);
            }
          }
          sphere(OR+IR);
        }
      }
      sphere(OR);
  }
}

module minkowskiOutsideRound(r=1,boundingEnvelope=[10000,10000,10000]){
    minkowski(){//expand the now positive shape
      difference(){//make the negative positive
        cube(boundingEnvelope-[0.1,0.1,0.1],center=true);
        minkowski(){//expand the negative inwards
          difference(){//create a negative of the children
            cube(boundingEnvelope,center=true);
            children();
          }
          sphere(r);
        }
      }
      sphere(r);
    }
}

module minkowskiInsideRound(r=1,boundingEnvelope=[10000,10000,10000]){
    difference(){//make the negative positive again
      cube(boundingEnvelope-[0.1,0.1,0.1],center=true);
      minkowski(){//expand the negative shape inwards
        difference(){//make the expanded children a negative shape
          cube(boundingEnvelope,center=true);
          minkowski(){//expand the children
            children();
            sphere(r);
          }
        }
        sphere(r);
      }
    }
}

module fillet(r=1,e=[10000,10000,10000]) {
  minkowskiRound(r,r,e) children();
}

module torus(r,o) {
  rotate_extrude(convexity=10) translate([r+o,0,0]) circle(r=r, $fn=$fn/4);
}



//parabola(1,0,0, .3, -2, 2, 20);

module parabola (p1, p2, p3, r, x0, x1, fn )
{
    s = (x1-x0)/fn;

    points = [ for (x=[x0:s:x1]) [x,p1*x*x+p2*x+p3,0] ];
    pipe_polygon(points, r=r, open=true, round_ends=false);
}

// OpenSCAD polygons have their radius/diameter
// measured to the vertices.  These functions
// give you adjusted r/d that will yield the
// specified r/d to the edges.
function adjustr(r, n) = r / cos(360/n/2);
function adjustd(d, n) = 2*adjustr(d/2, n);

function default(val, def) = (val != undef) ? val : def;

// @brief    Justify children as specified by argument.
// @param center   Equivalent to justify=[0,0,0].
// @param justify  [jx,jy,jz]:  1=toward-positive, 0=center, -1=toward-negative
// @param dims     Dimensions of child to justify
module justify(center, justify, dims) {
    function o(flag, dim) = flag > 0 ? 0 :
        flag < 0 ? -dim :
        -dim/2;
    j = default(justify, center ? [0,0,0] : [1,1,1]);
    translate([o(j[0], dims[0]),
        o(j[1], dims[1]),
        o(j[2], dims[2])]) children();
}

// Given a series of points, connect them with a pipe.
// This module has more features than are used in this model.
// poly - the array of points.
// r, d - the radius or diameter of the pipe.  Defaults to d=1.  (NB this is measured to the edges.)
// fill - if true, fill the interior of the figure, yielding a
// filled polygon with rounded edges.  Requires open=false.  Defaults to false.
// open - if false, connect the start and end points to close
// the polygon.  Defaults to false.
// justify - justify the polygon as specified.  Defaults to [1,1,1].
// round_ends - if true, cap the ends.  Defaults to true.
// $fn - controls smoothness at corners.  Defaults to 32.
// $fnz - the $fn value to use for the cylinder and the z-curves at the
// corners.  This lets you make pipes with polygonal cross-sections.  Defaults to $fn.
// NEEDSWORK:  $fa, $fs, and $fn should control the smoothness at corners.
module pipe_polygon(poly, r, d, fill=false, open=false, justify=[1,1,1], round_ends=true) {
    fn = $fn ? $fn : 32;
    fnz = is_undef($fnz) ? fn : $fnz;
    dims = [
        max([for (p=poly) p[0]]),
        max([for (p=poly) p[1]]),
        0
    ];
    _r = default(r, default(d,1)/2);
    radjusted = adjustr(_r, fnz);
    //justify(justify=justify, dims=dims) {
        $fn = fn;
        $fnz = fnz;
        for (i = [(round_ends ? 0 : 1) : len(poly)-(round_ends ? 1 : 2)]) {
            p = poly[i];
            translate([p[0], p[1], 0]) corner(r=radjusted);
        }
        poly2 = open ? poly : concat(poly, [poly[0]]);
        for (i=[0:len(poly2)-2]) {
            p1 = poly2[i];
            p2 = poly2[i+1];
            dy = p2[1] - p1[1];
            dx = p2[0] - p1[0];
            translate([p1[0], p1[1], 0]) {
                rotate([0,90,atan2(dy, dx)])
                rotate([0,0,360/fnz/2])
                    cylinder(r=radjusted, h=norm([dx, dy]), $fn=$fnz);
            }
        }
        if (fill) linear_extrude(_r*2, center=true) polygon(poly);
    //}
    // corner() defaults to a slice of a spheroid.  It's done
    // this way so that it can have different smoothing in x-y
    // and z, so that it can mate with a polygonal pipe.
    module corner(r, a) {
        rotate_extrude(angle=a) {
            intersection() {
                rotate([0,0,360/$fnz/2])
                    circle(r=r, $fn=$fnz);
                translate([0,-r*2])
                    square([r*2, r*4]);
            }
        }
    }
}


module BezConic(p0,p1,p2,steps=5) {
    /*
    http://www.thingiverse.com/thing:8931
    Conic Bezier Curve
    also known as Quadratic Bezier Curve
    also known as Bezier Curve with 3 control points

    Please see
    http://www.thingiverse.com/thing:8443 by William A Adams
    http://en.wikipedia.org/wiki/File:Bezier_2_big.gif by Phil Tregoning
    http://en.wikipedia.org/wiki/B%C3%A9zier_curve by Wikipedia editors

    By Don B, 2011, released into the Public Domain
    */

    stepsize1 = (p1-p0)/steps;
    stepsize2 = (p2-p1)/steps;

    for (i=[0:steps-1]) {
        assign(point1 = p0+stepsize1*i)
        assign(point2 = p1+stepsize2*i)
        assign(point3 = p0+stepsize1*(i+1))
        assign(point4 = p1+stepsize2*(i+1))  {
            assign( bpoint1 = point1+(point2-point1)*(i/steps) )
            assign( bpoint2 = point3+(point4-point3)*((i+1)/steps) ) {
                polygon(points=[bpoint1,bpoint2,p1]);
            }
        }
    }
}

module BezCone(r="Null", d=30, h=40, curve=-3, curve2="Null", steps=50) {
    /*
    Based on this Bezier function: http://www.thingiverse.com/thing:8931
    r, d, h act as you would expect.
    curve sets the amount of curve (actually sets x value for control point):
        - negative value gives concave surface
        - positive value gives convex surface
    curve2 sets the height (y) of the curve control point:
        - defaults to h/2
        - set to 0 to make curve onto base smooth
        - set to same value as h when convex to make top smooth, not pointed
    Some errors are caught and echoed to console, some are not.
    If it gives unexpected results, try fiddling with the values a little.

    AJC, August 2014, released into the Public Domain
    */
    d = (r=="Null") ? d : r*2;
    curve2 = (curve2=="Null") ? h/2 : curve2;
    p0 = [d/2, 0];
    p1 = [d/4+curve, curve2];
    p2 = [0, h];
    if( p1[0] < d/4 ) { //concave
        rotate_extrude($fn=steps)  {
            union() {
                polygon(points=[[0,0],p0,p1,p2,[0,h]]);
                BezConic(p0,p1,p2,steps);
            }
        }
    }
    if( p1[0] > d/4) { //convex
        rotate_extrude($fn=steps) {
            difference() {
                polygon(points=[[0,0],p0,p1,p2,[0,h]]);
                BezConic(p0,p1,p2,steps);
            }
        }
    }
    if( p1[0] == d/4) {
        echo("ERROR, BezCone, this will produce a cone, use cylinder instead!");
    }
    if( p1[0] < 0) {
        echo("ERROR, BezCone, curve cannot be less than radius/2");
    }
}
