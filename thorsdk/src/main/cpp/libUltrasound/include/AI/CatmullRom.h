//
// Copyright 2019 EchoNous Inc.
//

// Defines an extended Catmull-Rom spline passing through a
// list of control points (at least 4)

#pragma once
#include <vector>
#include <utility>
#include <geometry.h>


class CatmullRom
{
	// control points (min 4)
	std::vector<vec2> points;
	// coefficients for each segment
	//  length = 4*numSegments, where numSegments = points.length()-3
	std::vector<vec2> coeffs;
public:

	enum IntersectResult
	{
		INTERSECT,
		NO_INTERSECT,
		LOCAL_MIN
	};

	// empty spline, must call build()
	CatmullRom() = default;

	// Build a spline out of the given control points
	template <typename It>
	CatmullRom(It begin, It end);

	// Build a spline out of the given control points
	//  Replaces any existing data
	template <typename It>
	void build(It begin, It end);

	// evaluate the spline at a time t (ranges from 0 to 1)
	vec2 eval(float t) const;

	// evaluate the derivate at a time t
	vec2 deriv(float t) const;

	// find closest intersection time for a line
	//  uses newton's method, starting at a guess t
	//  intersection result is stored in loc
	IntersectResult intersect(vec2 p, vec2 n, float &t, vec2 &loc) const;

private:
	// get the segment index and segment time for time t
	std::pair<int,float> segment(float t) const;

	vec2 evalSegment(int segment, float t) const;
	vec2 derivSegment(int segment, float t) const;
};

#include "CatmullRom.inl"