//
// Copyright 2019 EchoNous Inc.
//

// Implementation for CatmullRom.h

#include <limits>
#include <cstdio>
#include <CatmullRom.h>


vec2 CatmullRom::eval(float t) const
{
	// get segment and segment time
	auto st = segment(t);
	return evalSegment(st.first, st.second);
}

vec2 CatmullRom::deriv(float t) const
{
	auto st = segment(t);
	// scale the derivative by the number of segments
	//  (because the parameter t is not the same as the t value passed into derivSegment)
	return derivSegment(st.first, st.second) * static_cast<float>(points.size()-3);
}

CatmullRom::IntersectResult CatmullRom::intersect(vec2 p, vec2 n, float &t, vec2 &loc) const
{
	// Lazy option, just select the closest intersection on the same side of the 0.5 divide..
	float start = t < 0.5f ? 0.0f : 0.5f;
	float end = start + 0.5f;

	float minh = 1000.0f;
	const int ITERATIONS = 300;
	for (int i=0; i < ITERATIONS; ++i)
	{
		float ti = start + 0.5f/ITERATIONS * i;
		vec2 loci = eval(ti);

		float h = std::abs(dot(n, loci-p));
		if (h < minh) {
			t = ti;
			loc = loci;
			minh = h;
		}
	}



	// Because we are working in image space, a difference of less than a pixel is "perfect"
	if (minh < 1.0f)
		return INTERSECT;
	else
		return NO_INTERSECT;

	/*
	// BUG: this intersection algorithm is not as robust as it really needs to be
	//   if any of the spline walls get close to parallel with the disks, then the algorithm tends to fail
	//   one fix idea is to provide an upper and lower bound to t, then use bisection to narrow down the possible location
	int iterations = 0;
	const int maxIterations = 100;
	do
	{
		// get spline location at t
		loc = eval(t);
		vec2 dist = loc-p;

		// evaluate distance from line
		float h = dot(n, dist);
		if (std::abs(h) < 0.0001f)
			return INTERSECT; // found intersection!

		// evaluate dh/dt
		float dh = dot(n, deriv(t));

		// check for local minimum
		// BUG: This does a very poor job of detecting local minima, and will almost never be hit
		//  instead, maybe check if we "orbit" a point without h decreasing?
		if (std::abs(dh) < 0.0001f)
			return LOCAL_MIN;

		// step t according to newton's method
		t = t - h/dh;
		++iterations;
	} while (-0.0001f <= t && t <= 1.0001f && iterations < maxIterations);
	return NO_INTERSECT;
	*/
}

std::pair<int, float> CatmullRom::segment(float t) const
{
	int numSegments = points.size()-3;
	float stime = 1.0f/numSegments; // length of time per segment
	int s = t / stime; // which segment we are in
	// if the parameter t is exactly 1.0f, then we can hit this case
	if (s == (int)points.size()-3)
	{
		return std::make_pair(s-1, 1.0f);
	}
	float st = (t-s*stime) / stime; // where we are in segment
	return std::make_pair(s,st);
}

vec2 CatmullRom::evalSegment(int segment, float t) const
{
	auto a = coeffs[4*segment];
	auto b = coeffs[4*segment+1];
	auto c = coeffs[4*segment+2];
	auto d = coeffs[4*segment+3];

	return a*t*t*t + b*t*t + c*t + d;
}

vec2 CatmullRom::derivSegment(int segment, float t) const
{
	auto a = coeffs[4*segment];
	auto b = coeffs[4*segment+1];
	auto c = coeffs[4*segment+2];

	return 3.0f*a*t*t + 2.0f*b*t + c;
}