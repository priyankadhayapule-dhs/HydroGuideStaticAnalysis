//
// Copyright 2019 EchoNous Inc.
//

// Implementation for template functions in CatmullRom.h

template <typename It>
CatmullRom::CatmullRom(It begin, It end)
{
	build(begin, end);
}

template <typename It>
void CatmullRom::build(It begin, It end)
{
	// copy control points
	points.clear();
	std::copy(begin, end, std::back_inserter(points));
	coeffs.clear();
	// create coeffs
	for (unsigned i=0; i < points.size()-3; ++i)
	{
		const auto &p0 = points[i];
		const auto &p1 = points[i+1];
		const auto &p2 = points[i+2];
		const auto &p3 = points[i+3];
		coeffs.push_back(0.5f*(-p0+3.0f*p1-3.0f*p2+p3));
		coeffs.push_back(0.5f*(2.0f*p0-5.0f*p1+4.0f*p2-p3));
		coeffs.push_back(0.5f*(-p0+p2));
		coeffs.push_back(p1);
	}
}
