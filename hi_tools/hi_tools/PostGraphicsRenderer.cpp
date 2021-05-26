/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace OpenSimplexNoise
{
	class Noise
	{
	public:
		Noise();
		Noise(int64_t seed);
		//2D Open Simplex Noise.
		double eval(const double x, const double y) const;
		//3D Open Simplex Noise.
		double eval(double x, double y, double z) const;
		//4D Open Simplex Noise.
		double eval(double x, double y, double z, double w) const;
	private:
		const double m_stretch2d;
		const double m_squish2d;
		const double m_stretch3d;
		const double m_squish3d;
		const double m_stretch4d;
		const double m_squish4d;

		const double m_norm2d;
		const double m_norm3d;
		const double m_norm4d;

		const long m_defaultSeed;

		std::array<short, 256> m_perm;
		std::array<short, 256> m_permGradIndex3d;
		std::array<char, 16> m_gradients2d;
		std::array<char, 72> m_gradients3d;
		std::array<char, 256> m_gradients4d;
		double extrapolate(int xsb, int ysb, double dx, double dy) const;
		double extrapolate(int xsb, int ysb, int zsb, double dx, double dy, double dz) const;
		double extrapolate(int xsb, int ysb, int zsb, int wsb, double dx, double dy, double dz, double dw) const;
	};
}

namespace OpenSimplexNoise
{
	using namespace std;

	Noise::Noise()
		: m_stretch2d(-0.211324865405187) //(1/Math.sqrt(2+1)-1)/2;
		, m_squish2d(0.366025403784439)   //(Math.sqrt(2+1)-1)/2;
		, m_stretch3d(-1.0 / 6)           //(1/Math.sqrt(3+1)-1)/3;
		, m_squish3d(1.0 / 3)             //(Math.sqrt(3+1)-1)/3;
		, m_stretch4d(-0.138196601125011) //(1/Math.sqrt(4+1)-1)/4;
		, m_squish4d(0.309016994374947)   //(Math.sqrt(4+1)-1)/4;
		, m_norm2d(47)
		, m_norm3d(103)
		, m_norm4d(30)
		, m_defaultSeed(0)
		, m_perm{ 0 }
		, m_permGradIndex3d{ 0 }
		, m_gradients2d{ 5,  2,    2,  5,
						-5,  2,   -2,  5,
						 5, -2,    2, -5,
						-5, -2,   -2, -5, }
		, m_gradients3d{ -11,  4,  4,     -4,  11,  4,    -4,  4,  11,
						 11,  4,  4,      4,  11,  4,     4,  4,  11,
						-11, -4,  4,     -4, -11,  4,    -4, -4,  11,
						 11, -4,  4,      4, -11,  4,     4, -4,  11,
						-11,  4, -4,     -4,  11, -4,    -4,  4, -11,
						 11,  4, -4,      4,  11, -4,     4,  4, -11,
						-11, -4, -4,     -4, -11, -4,    -4, -4, -11,
						 11, -4, -4,      4, -11, -4,     4, -4, -11, }
		, m_gradients4d{ 3,  1,  1,  1,      1,  3,  1,  1,      1,  1,  3,  1,      1,  1,  1,  3,
						-3,  1,  1,  1,     -1,  3,  1,  1,     -1,  1,  3,  1,     -1,  1,  1,  3,
						 3, -1,  1,  1,      1, -3,  1,  1,      1, -1,  3,  1,      1, -1,  1,  3,
						-3, -1,  1,  1,     -1, -3,  1,  1,     -1, -1,  3,  1,     -1, -1,  1,  3,
						 3,  1, -1,  1,      1,  3, -1,  1,      1,  1, -3,  1,      1,  1, -1,  3,
						-3,  1, -1,  1,     -1,  3, -1,  1,     -1,  1, -3,  1,     -1,  1, -1,  3,
						 3, -1, -1,  1,      1, -3, -1,  1,      1, -1, -3,  1,      1, -1, -1,  3,
						-3, -1, -1,  1,     -1, -3, -1,  1,     -1, -1, -3,  1,     -1, -1, -1,  3,
						 3,  1,  1, -1,      1,  3,  1, -1,      1,  1,  3, -1,      1,  1,  1, -3,
						-3,  1,  1, -1,     -1,  3,  1, -1,     -1,  1,  3, -1,     -1,  1,  1, -3,
						 3, -1,  1, -1,      1, -3,  1, -1,      1, -1,  3, -1,      1, -1,  1, -3,
						-3, -1,  1, -1,     -1, -3,  1, -1,     -1, -1,  3, -1,     -1, -1,  1, -3,
						 3,  1, -1, -1,      1,  3, -1, -1,      1,  1, -3, -1,      1,  1, -1, -3,
						-3,  1, -1, -1,     -1,  3, -1, -1,     -1,  1, -3, -1,     -1,  1, -1, -3,
						 3, -1, -1, -1,      1, -3, -1, -1,      1, -1, -3, -1,      1, -1, -1, -3,
						-3, -1, -1, -1,     -1, -3, -1, -1,     -1, -1, -3, -1,     -1, -1, -1, -3, }
	{
	}

	Noise::Noise(int64_t seed)
		: Noise()
	{
		short source[256];
		for (short i = 0; i < 256; i++)
		{
			source[i] = i;
		}
		seed = seed * 6364136223846793005l + 1442695040888963407l;
		seed = seed * 6364136223846793005l + 1442695040888963407l;
		seed = seed * 6364136223846793005l + 1442695040888963407l;
		for (int i = 255; i >= 0; i--)
		{
			seed = seed * 6364136223846793005l + 1442695040888963407l;
			int r = static_cast<int>((seed + 31) % (i + 1));
			if (r < 0)
			{
				r += (i + 1);
			}
			m_perm[i] = source[r];
			m_permGradIndex3d[i] = static_cast<short>((m_perm[i] % (m_gradients3d.size() / 3)) * 3);
			source[r] = source[i];
		}
	}

	double Noise::eval(double x, double y) const
	{
		//Place input coordinates onto grid.
		double stretchOffset = (x + y) * m_stretch2d;
		double xs = x + stretchOffset;
		double ys = y + stretchOffset;

		//Floor to get grid coordinates of rhombus (stretched square) super-cell origin.
		int xsb = static_cast<int>(floor(xs));
		int ysb = static_cast<int>(floor(ys));

		//Skew out to get actual coordinates of rhombus origin. We'll need these later.
		double squishOffset = (xsb + ysb) * m_squish2d;
		double xb = xsb + squishOffset;
		double yb = ysb + squishOffset;

		//Compute grid coordinates relative to rhombus origin.
		double xins = xs - xsb;
		double yins = ys - ysb;

		//Sum those together to get a value that determines which region we're in.
		double inSum = xins + yins;

		//Positions relative to origin point.
		double dx0 = x - xb;
		double dy0 = y - yb;

		//We'll be defining these inside the next block and using them afterwards.
		double dx_ext, dy_ext;
		int xsv_ext, ysv_ext;

		double value = 0;

		//Contribution (1,0)
		double dx1 = dx0 - 1 - m_squish2d;
		double dy1 = dy0 - 0 - m_squish2d;
		double attn1 = 2 - dx1 * dx1 - dy1 * dy1;
		if (attn1 > 0)
		{
			attn1 *= attn1;
			value += attn1 * attn1 * extrapolate(xsb + 1, ysb + 0, dx1, dy1);
		}

		//Contribution (0,1)
		double dx2 = dx0 - 0 - m_squish2d;
		double dy2 = dy0 - 1 - m_squish2d;
		double attn2 = 2 - dx2 * dx2 - dy2 * dy2;
		if (attn2 > 0)
		{
			attn2 *= attn2;
			value += attn2 * attn2 * extrapolate(xsb + 0, ysb + 1, dx2, dy2);
		}

		if (inSum <= 1)
		{ //We're inside the triangle (2-Simplex) at (0,0)
			double zins = 1 - inSum;
			if (zins > xins || zins > yins)
			{ //(0,0) is one of the closest two triangular vertices
				if (xins > yins)
				{
					xsv_ext = xsb + 1;
					ysv_ext = ysb - 1;
					dx_ext = dx0 - 1;
					dy_ext = dy0 + 1;
				}
				else
				{
					xsv_ext = xsb - 1;
					ysv_ext = ysb + 1;
					dx_ext = dx0 + 1;
					dy_ext = dy0 - 1;
				}
			}
			else
			{ //(1,0) and (0,1) are the closest two vertices.
				xsv_ext = xsb + 1;
				ysv_ext = ysb + 1;
				dx_ext = dx0 - 1 - 2 * m_squish2d;
				dy_ext = dy0 - 1 - 2 * m_squish2d;
			}
		}
		else
		{ //We're inside the triangle (2-Simplex) at (1,1)
			double zins = 2 - inSum;
			if (zins < xins || zins < yins)
			{ //(0,0) is one of the closest two triangular vertices
				if (xins > yins)
				{
					xsv_ext = xsb + 2;
					ysv_ext = ysb + 0;
					dx_ext = dx0 - 2 - 2 * m_squish2d;
					dy_ext = dy0 + 0 - 2 * m_squish2d;
				}
				else
				{
					xsv_ext = xsb + 0;
					ysv_ext = ysb + 2;
					dx_ext = dx0 + 0 - 2 * m_squish2d;
					dy_ext = dy0 - 2 - 2 * m_squish2d;
				}
			}
			else
			{ //(1,0) and (0,1) are the closest two vertices.
				dx_ext = dx0;
				dy_ext = dy0;
				xsv_ext = xsb;
				ysv_ext = ysb;
			}
			xsb += 1;
			ysb += 1;
			dx0 = dx0 - 1 - 2 * m_squish2d;
			dy0 = dy0 - 1 - 2 * m_squish2d;
		}

		//Contribution (0,0) or (1,1)
		double attn0 = 2 - dx0 * dx0 - dy0 * dy0;
		if (attn0 > 0)
		{
			attn0 *= attn0;
			value += attn0 * attn0 * extrapolate(xsb, ysb, dx0, dy0);
		}

		//Extra Vertex
		double attn_ext = 2 - dx_ext * dx_ext - dy_ext * dy_ext;
		if (attn_ext > 0)
		{
			attn_ext *= attn_ext;
			value += attn_ext * attn_ext * extrapolate(xsv_ext, ysv_ext, dx_ext, dy_ext);
		}

		return value / m_norm2d;
	}

	double Noise::eval(double x, double y, double z) const
	{
		//Place input coordinates on simplectic honeycomb.
		double stretchOffset = (x + y + z) * m_stretch3d;
		double xs = x + stretchOffset;
		double ys = y + stretchOffset;
		double zs = z + stretchOffset;

		//static_cast<int>(floor to get simplectic honeycomb coordinates of rhombohedron (stretched cube) super-cell origin.
		int xsb = static_cast<int>(floor(xs));
		int ysb = static_cast<int>(floor(ys));
		int zsb = static_cast<int>(floor(zs));

		//Skew out to get actual coordinates of rhombohedron origin. We'll need these later.
		double squishOffset = (xsb + ysb + zsb) * m_squish3d;
		double xb = xsb + squishOffset;
		double yb = ysb + squishOffset;
		double zb = zsb + squishOffset;

		//Compute simplectic honeycomb coordinates relative to rhombohedral origin.
		double xins = xs - xsb;
		double yins = ys - ysb;
		double zins = zs - zsb;

		//Sum those together to get a value that determines which region we're in.
		double inSum = xins + yins + zins;

		//Positions relative to origin point.
		double dx0 = x - xb;
		double dy0 = y - yb;
		double dz0 = z - zb;

		//We'll be defining these inside the next block and using them afterwards.
		double dx_ext0, dy_ext0, dz_ext0;
		double dx_ext1, dy_ext1, dz_ext1;
		int xsv_ext0, ysv_ext0, zsv_ext0;
		int xsv_ext1, ysv_ext1, zsv_ext1;

		double value = 0;
		if (inSum <= 1)
		{ //We're inside the tetrahedron (3-Simplex) at (0,0,0)

	//Determine which two of (0,0,1), (0,1,0), (1,0,0) are closest.
			char aPoint = 0x01;
			double aScore = xins;
			char bPoint = 0x02;
			double bScore = yins;
			if (aScore >= bScore && zins > bScore)
			{
				bScore = zins;
				bPoint = 0x04;
			}
			else if (aScore < bScore && zins > aScore)
			{
				aScore = zins;
				aPoint = 0x04;
			}

			//Now we determine the two lattice points not part of the tetrahedron that may contribute.
			//This depends on the closest two tetrahedral vertices, including (0,0,0)
			double wins = 1 - inSum;
			if (wins > aScore || wins > bScore)
			{ //(0,0,0) is one of the closest two tetrahedral vertices.
				char c = (bScore > aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.

				if ((c & 0x01) == 0)
				{
					xsv_ext0 = xsb - 1;
					xsv_ext1 = xsb;
					dx_ext0 = dx0 + 1;
					dx_ext1 = dx0;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsb + 1;
					dx_ext0 = dx_ext1 = dx0 - 1;
				}

				if ((c & 0x02) == 0)
				{
					ysv_ext0 = ysv_ext1 = ysb;
					dy_ext0 = dy_ext1 = dy0;
					if ((c & 0x01) == 0)
					{
						ysv_ext1 -= 1;
						dy_ext1 += 1;
					}
					else
					{
						ysv_ext0 -= 1;
						dy_ext0 += 1;
					}
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysb + 1;
					dy_ext0 = dy_ext1 = dy0 - 1;
				}

				if ((c & 0x04) == 0)
				{
					zsv_ext0 = zsb;
					zsv_ext1 = zsb - 1;
					dz_ext0 = dz0;
					dz_ext1 = dz0 + 1;
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsb + 1;
					dz_ext0 = dz_ext1 = dz0 - 1;
				}
			}
			else
			{ //(0,0,0) is not one of the closest two tetrahedral vertices.
				char c = static_cast<char>(aPoint | bPoint); //Our two extra vertices are determined by the closest two.

				if ((c & 0x01) == 0)
				{
					xsv_ext0 = xsb;
					xsv_ext1 = xsb - 1;
					dx_ext0 = dx0 - 2 * m_squish3d;
					dx_ext1 = dx0 + 1 - m_squish3d;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsb + 1;
					dx_ext0 = dx0 - 1 - 2 * m_squish3d;
					dx_ext1 = dx0 - 1 - m_squish3d;
				}

				if ((c & 0x02) == 0)
				{
					ysv_ext0 = ysb;
					ysv_ext1 = ysb - 1;
					dy_ext0 = dy0 - 2 * m_squish3d;
					dy_ext1 = dy0 + 1 - m_squish3d;
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysb + 1;
					dy_ext0 = dy0 - 1 - 2 * m_squish3d;
					dy_ext1 = dy0 - 1 - m_squish3d;
				}

				if ((c & 0x04) == 0)
				{
					zsv_ext0 = zsb;
					zsv_ext1 = zsb - 1;
					dz_ext0 = dz0 - 2 * m_squish3d;
					dz_ext1 = dz0 + 1 - m_squish3d;
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsb + 1;
					dz_ext0 = dz0 - 1 - 2 * m_squish3d;
					dz_ext1 = dz0 - 1 - m_squish3d;
				}
			}

			//Contribution (0,0,0)
			double attn0 = 2 - dx0 * dx0 - dy0 * dy0 - dz0 * dz0;
			if (attn0 > 0)
			{
				attn0 *= attn0;
				value += attn0 * attn0 * extrapolate(xsb + 0, ysb + 0, zsb + 0, dx0, dy0, dz0);
			}

			//Contribution (1,0,0)
			double dx1 = dx0 - 1 - m_squish3d;
			double dy1 = dy0 - 0 - m_squish3d;
			double dz1 = dz0 - 0 - m_squish3d;
			double attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1;
			if (attn1 > 0)
			{
				attn1 *= attn1;
				value += attn1 * attn1 * extrapolate(xsb + 1, ysb + 0, zsb + 0, dx1, dy1, dz1);
			}

			//Contribution (0,1,0)
			double dx2 = dx0 - 0 - m_squish3d;
			double dy2 = dy0 - 1 - m_squish3d;
			double dz2 = dz1;
			double attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2;
			if (attn2 > 0)
			{
				attn2 *= attn2;
				value += attn2 * attn2 * extrapolate(xsb + 0, ysb + 1, zsb + 0, dx2, dy2, dz2);
			}

			//Contribution (0,0,1)
			double dx3 = dx2;
			double dy3 = dy1;
			double dz3 = dz0 - 1 - m_squish3d;
			double attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3;
			if (attn3 > 0)
			{
				attn3 *= attn3;
				value += attn3 * attn3 * extrapolate(xsb + 0, ysb + 0, zsb + 1, dx3, dy3, dz3);
			}
		}
		else if (inSum >= 2)
		{ //We're inside the tetrahedron (3-Simplex) at (1,1,1)

	//Determine which two tetrahedral vertices are the closest, out of (1,1,0), (1,0,1), (0,1,1) but not (1,1,1).
			char aPoint = 0x06;
			double aScore = xins;
			char bPoint = 0x05;
			double bScore = yins;
			if (aScore <= bScore && zins < bScore)
			{
				bScore = zins;
				bPoint = 0x03;
			}
			else if (aScore > bScore && zins < aScore)
			{
				aScore = zins;
				aPoint = 0x03;
			}

			//Now we determine the two lattice points not part of the tetrahedron that may contribute.
			//This depends on the closest two tetrahedral vertices, including (1,1,1)
			double wins = 3 - inSum;
			if (wins < aScore || wins < bScore)
			{ //(1,1,1) is one of the closest two tetrahedral vertices.
				char c = (bScore < aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.

				if ((c & 0x01) != 0)
				{
					xsv_ext0 = xsb + 2;
					xsv_ext1 = xsb + 1;
					dx_ext0 = dx0 - 2 - 3 * m_squish3d;
					dx_ext1 = dx0 - 1 - 3 * m_squish3d;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsb;
					dx_ext0 = dx_ext1 = dx0 - 3 * m_squish3d;
				}

				if ((c & 0x02) != 0)
				{
					ysv_ext0 = ysv_ext1 = ysb + 1;
					dy_ext0 = dy_ext1 = dy0 - 1 - 3 * m_squish3d;
					if ((c & 0x01) != 0)
					{
						ysv_ext1 += 1;
						dy_ext1 -= 1;
					}
					else
					{
						ysv_ext0 += 1;
						dy_ext0 -= 1;
					}
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysb;
					dy_ext0 = dy_ext1 = dy0 - 3 * m_squish3d;
				}

				if ((c & 0x04) != 0)
				{
					zsv_ext0 = zsb + 1;
					zsv_ext1 = zsb + 2;
					dz_ext0 = dz0 - 1 - 3 * m_squish3d;
					dz_ext1 = dz0 - 2 - 3 * m_squish3d;
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsb;
					dz_ext0 = dz_ext1 = dz0 - 3 * m_squish3d;
				}
			}
			else
			{ //(1,1,1) is not one of the closest two tetrahedral vertices.
				char c = static_cast<char>(aPoint & bPoint); //Our two extra vertices are determined by the closest two.

				if ((c & 0x01) != 0)
				{
					xsv_ext0 = xsb + 1;
					xsv_ext1 = xsb + 2;
					dx_ext0 = dx0 - 1 - m_squish3d;
					dx_ext1 = dx0 - 2 - 2 * m_squish3d;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsb;
					dx_ext0 = dx0 - m_squish3d;
					dx_ext1 = dx0 - 2 * m_squish3d;
				}

				if ((c & 0x02) != 0)
				{
					ysv_ext0 = ysb + 1;
					ysv_ext1 = ysb + 2;
					dy_ext0 = dy0 - 1 - m_squish3d;
					dy_ext1 = dy0 - 2 - 2 * m_squish3d;
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysb;
					dy_ext0 = dy0 - m_squish3d;
					dy_ext1 = dy0 - 2 * m_squish3d;
				}

				if ((c & 0x04) != 0)
				{
					zsv_ext0 = zsb + 1;
					zsv_ext1 = zsb + 2;
					dz_ext0 = dz0 - 1 - m_squish3d;
					dz_ext1 = dz0 - 2 - 2 * m_squish3d;
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsb;
					dz_ext0 = dz0 - m_squish3d;
					dz_ext1 = dz0 - 2 * m_squish3d;
				}
			}

			//Contribution (1,1,0)
			double dx3 = dx0 - 1 - 2 * m_squish3d;
			double dy3 = dy0 - 1 - 2 * m_squish3d;
			double dz3 = dz0 - 0 - 2 * m_squish3d;
			double attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3;
			if (attn3 > 0)
			{
				attn3 *= attn3;
				value += attn3 * attn3 * extrapolate(xsb + 1, ysb + 1, zsb + 0, dx3, dy3, dz3);
			}

			//Contribution (1,0,1)
			double dx2 = dx3;
			double dy2 = dy0 - 0 - 2 * m_squish3d;
			double dz2 = dz0 - 1 - 2 * m_squish3d;
			double attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2;
			if (attn2 > 0)
			{
				attn2 *= attn2;
				value += attn2 * attn2 * extrapolate(xsb + 1, ysb + 0, zsb + 1, dx2, dy2, dz2);
			}

			//Contribution (0,1,1)
			double dx1 = dx0 - 0 - 2 * m_squish3d;
			double dy1 = dy3;
			double dz1 = dz2;
			double attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1;
			if (attn1 > 0)
			{
				attn1 *= attn1;
				value += attn1 * attn1 * extrapolate(xsb + 0, ysb + 1, zsb + 1, dx1, dy1, dz1);
			}

			//Contribution (1,1,1)
			dx0 = dx0 - 1 - 3 * m_squish3d;
			dy0 = dy0 - 1 - 3 * m_squish3d;
			dz0 = dz0 - 1 - 3 * m_squish3d;
			double attn0 = 2 - dx0 * dx0 - dy0 * dy0 - dz0 * dz0;
			if (attn0 > 0)
			{
				attn0 *= attn0;
				value += attn0 * attn0 * extrapolate(xsb + 1, ysb + 1, zsb + 1, dx0, dy0, dz0);
			}
		}
		else
		{ //We're inside the octahedron (Rectified 3-Simplex) in between.
			double aScore;
			char aPoint;
			bool aIsFurtherSide;
			double bScore;
			char bPoint;
			bool bIsFurtherSide;

			//Decide between point (0,0,1) and (1,1,0) as closest
			double p1 = xins + yins;
			if (p1 > 1)
			{
				aScore = p1 - 1;
				aPoint = 0x03;
				aIsFurtherSide = true;
			}
			else
			{
				aScore = 1 - p1;
				aPoint = 0x04;
				aIsFurtherSide = false;
			}

			//Decide between point (0,1,0) and (1,0,1) as closest
			double p2 = xins + zins;
			if (p2 > 1)
			{
				bScore = p2 - 1;
				bPoint = 0x05;
				bIsFurtherSide = true;
			}
			else
			{
				bScore = 1 - p2;
				bPoint = 0x02;
				bIsFurtherSide = false;
			}

			//The closest out of the two (1,0,0) and (0,1,1) will replace the furthest out of the two decided above, if closer.
			double p3 = yins + zins;
			if (p3 > 1)
			{
				double score = p3 - 1;
				if (aScore <= bScore && aScore < score)
				{
					aScore = score;
					aPoint = 0x06;
					aIsFurtherSide = true;
				}
				else if (aScore > bScore && bScore < score)
				{
					bScore = score;
					bPoint = 0x06;
					bIsFurtherSide = true;
				}
			}
			else
			{
				double score = 1 - p3;
				if (aScore <= bScore && aScore < score)
				{
					aScore = score;
					aPoint = 0x01;
					aIsFurtherSide = false;
				}
				else if (aScore > bScore && bScore < score)
				{
					bScore = score;
					bPoint = 0x01;
					bIsFurtherSide = false;
				}
			}

			//Where each of the two closest points are determines how the extra two vertices are calculated.
			if (aIsFurtherSide == bIsFurtherSide)
			{
				if (aIsFurtherSide)
				{ //Both closest points on (1,1,1) side

		//One of the two extra points is (1,1,1)
					dx_ext0 = dx0 - 1 - 3 * m_squish3d;
					dy_ext0 = dy0 - 1 - 3 * m_squish3d;
					dz_ext0 = dz0 - 1 - 3 * m_squish3d;
					xsv_ext0 = xsb + 1;
					ysv_ext0 = ysb + 1;
					zsv_ext0 = zsb + 1;

					//Other extra point is based on the shared axis.
					char c = static_cast<char>(aPoint & bPoint);
					if ((c & 0x01) != 0)
					{
						dx_ext1 = dx0 - 2 - 2 * m_squish3d;
						dy_ext1 = dy0 - 2 * m_squish3d;
						dz_ext1 = dz0 - 2 * m_squish3d;
						xsv_ext1 = xsb + 2;
						ysv_ext1 = ysb;
						zsv_ext1 = zsb;
					}
					else if ((c & 0x02) != 0)
					{
						dx_ext1 = dx0 - 2 * m_squish3d;
						dy_ext1 = dy0 - 2 - 2 * m_squish3d;
						dz_ext1 = dz0 - 2 * m_squish3d;
						xsv_ext1 = xsb;
						ysv_ext1 = ysb + 2;
						zsv_ext1 = zsb;
					}
					else
					{
						dx_ext1 = dx0 - 2 * m_squish3d;
						dy_ext1 = dy0 - 2 * m_squish3d;
						dz_ext1 = dz0 - 2 - 2 * m_squish3d;
						xsv_ext1 = xsb;
						ysv_ext1 = ysb;
						zsv_ext1 = zsb + 2;
					}
				}
				else
				{//Both closest points on (0,0,0) side

				   //One of the two extra points is (0,0,0)
					dx_ext0 = dx0;
					dy_ext0 = dy0;
					dz_ext0 = dz0;
					xsv_ext0 = xsb;
					ysv_ext0 = ysb;
					zsv_ext0 = zsb;

					//Other extra point is based on the omitted axis.
					char c = static_cast<char>(aPoint | bPoint);
					if ((c & 0x01) == 0)
					{
						dx_ext1 = dx0 + 1 - m_squish3d;
						dy_ext1 = dy0 - 1 - m_squish3d;
						dz_ext1 = dz0 - 1 - m_squish3d;
						xsv_ext1 = xsb - 1;
						ysv_ext1 = ysb + 1;
						zsv_ext1 = zsb + 1;
					}
					else if ((c & 0x02) == 0)
					{
						dx_ext1 = dx0 - 1 - m_squish3d;
						dy_ext1 = dy0 + 1 - m_squish3d;
						dz_ext1 = dz0 - 1 - m_squish3d;
						xsv_ext1 = xsb + 1;
						ysv_ext1 = ysb - 1;
						zsv_ext1 = zsb + 1;
					}
					else
					{
						dx_ext1 = dx0 - 1 - m_squish3d;
						dy_ext1 = dy0 - 1 - m_squish3d;
						dz_ext1 = dz0 + 1 - m_squish3d;
						xsv_ext1 = xsb + 1;
						ysv_ext1 = ysb + 1;
						zsv_ext1 = zsb - 1;
					}
				}
			}
			else
			{ //One point on (0,0,0) side, one point on (1,1,1) side
				char c1, c2;
				if (aIsFurtherSide)
				{
					c1 = aPoint;
					c2 = bPoint;
				}
				else
				{
					c1 = bPoint;
					c2 = aPoint;
				}

				//One contribution is a permutation of (1,1,-1)
				if ((c1 & 0x01) == 0)
				{
					dx_ext0 = dx0 + 1 - m_squish3d;
					dy_ext0 = dy0 - 1 - m_squish3d;
					dz_ext0 = dz0 - 1 - m_squish3d;
					xsv_ext0 = xsb - 1;
					ysv_ext0 = ysb + 1;
					zsv_ext0 = zsb + 1;
				}
				else if ((c1 & 0x02) == 0)
				{
					dx_ext0 = dx0 - 1 - m_squish3d;
					dy_ext0 = dy0 + 1 - m_squish3d;
					dz_ext0 = dz0 - 1 - m_squish3d;
					xsv_ext0 = xsb + 1;
					ysv_ext0 = ysb - 1;
					zsv_ext0 = zsb + 1;
				}
				else
				{
					dx_ext0 = dx0 - 1 - m_squish3d;
					dy_ext0 = dy0 - 1 - m_squish3d;
					dz_ext0 = dz0 + 1 - m_squish3d;
					xsv_ext0 = xsb + 1;
					ysv_ext0 = ysb + 1;
					zsv_ext0 = zsb - 1;
				}

				//One contribution is a permutation of (0,0,2)
				dx_ext1 = dx0 - 2 * m_squish3d;
				dy_ext1 = dy0 - 2 * m_squish3d;
				dz_ext1 = dz0 - 2 * m_squish3d;
				xsv_ext1 = xsb;
				ysv_ext1 = ysb;
				zsv_ext1 = zsb;
				if ((c2 & 0x01) != 0)
				{
					dx_ext1 -= 2;
					xsv_ext1 += 2;
				}
				else if ((c2 & 0x02) != 0)
				{
					dy_ext1 -= 2;
					ysv_ext1 += 2;
				}
				else
				{
					dz_ext1 -= 2;
					zsv_ext1 += 2;
				}
			}

			//Contribution (1,0,0)
			double dx1 = dx0 - 1 - m_squish3d;
			double dy1 = dy0 - 0 - m_squish3d;
			double dz1 = dz0 - 0 - m_squish3d;
			double attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1;
			if (attn1 > 0)
			{
				attn1 *= attn1;
				value += attn1 * attn1 * extrapolate(xsb + 1, ysb + 0, zsb + 0, dx1, dy1, dz1);
			}

			//Contribution (0,1,0)
			double dx2 = dx0 - 0 - m_squish3d;
			double dy2 = dy0 - 1 - m_squish3d;
			double dz2 = dz1;
			double attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2;
			if (attn2 > 0)
			{
				attn2 *= attn2;
				value += attn2 * attn2 * extrapolate(xsb + 0, ysb + 1, zsb + 0, dx2, dy2, dz2);
			}

			//Contribution (0,0,1)
			double dx3 = dx2;
			double dy3 = dy1;
			double dz3 = dz0 - 1 - m_squish3d;
			double attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3;
			if (attn3 > 0)
			{
				attn3 *= attn3;
				value += attn3 * attn3 * extrapolate(xsb + 0, ysb + 0, zsb + 1, dx3, dy3, dz3);
			}

			//Contribution (1,1,0)
			double dx4 = dx0 - 1 - 2 * m_squish3d;
			double dy4 = dy0 - 1 - 2 * m_squish3d;
			double dz4 = dz0 - 0 - 2 * m_squish3d;
			double attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4;
			if (attn4 > 0)
			{
				attn4 *= attn4;
				value += attn4 * attn4 * extrapolate(xsb + 1, ysb + 1, zsb + 0, dx4, dy4, dz4);
			}

			//Contribution (1,0,1)
			double dx5 = dx4;
			double dy5 = dy0 - 0 - 2 * m_squish3d;
			double dz5 = dz0 - 1 - 2 * m_squish3d;
			double attn5 = 2 - dx5 * dx5 - dy5 * dy5 - dz5 * dz5;
			if (attn5 > 0)
			{
				attn5 *= attn5;
				value += attn5 * attn5 * extrapolate(xsb + 1, ysb + 0, zsb + 1, dx5, dy5, dz5);
			}

			//Contribution (0,1,1)
			double dx6 = dx0 - 0 - 2 * m_squish3d;
			double dy6 = dy4;
			double dz6 = dz5;
			double attn6 = 2 - dx6 * dx6 - dy6 * dy6 - dz6 * dz6;
			if (attn6 > 0)
			{
				attn6 *= attn6;
				value += attn6 * attn6 * extrapolate(xsb + 0, ysb + 1, zsb + 1, dx6, dy6, dz6);
			}
		}

		//First extra vertex
		double attn_ext0 = 2 - dx_ext0 * dx_ext0 - dy_ext0 * dy_ext0 - dz_ext0 * dz_ext0;
		if (attn_ext0 > 0)
		{
			attn_ext0 *= attn_ext0;
			value += attn_ext0 * attn_ext0 * extrapolate(xsv_ext0, ysv_ext0, zsv_ext0, dx_ext0, dy_ext0, dz_ext0);
		}

		//Second extra vertex
		double attn_ext1 = 2 - dx_ext1 * dx_ext1 - dy_ext1 * dy_ext1 - dz_ext1 * dz_ext1;
		if (attn_ext1 > 0)
		{
			attn_ext1 *= attn_ext1;
			value += attn_ext1 * attn_ext1 * extrapolate(xsv_ext1, ysv_ext1, zsv_ext1, dx_ext1, dy_ext1, dz_ext1);
		}

		return value / m_norm3d;
	}

	double Noise::eval(double x, double y, double z, double w) const
	{
		//Place input coordinates on simplectic honeycomb.
		double stretchOffset = (x + y + z + w) * m_stretch4d;
		double xs = x + stretchOffset;
		double ys = y + stretchOffset;
		double zs = z + stretchOffset;
		double ws = w + stretchOffset;

		//static_cast<int>(floor to get simplectic honeycomb coordinates of rhombo-hypercube super-cell origin.
		int xsb = static_cast<int>(floor(xs));
		int ysb = static_cast<int>(floor(ys));
		int zsb = static_cast<int>(floor(zs));
		int wsb = static_cast<int>(floor(ws));

		//Skew out to get actual coordinates of stretched rhombo-hypercube origin. We'll need these later.
		double squishOffset = (xsb + ysb + zsb + wsb) * m_squish4d;
		double xb = xsb + squishOffset;
		double yb = ysb + squishOffset;
		double zb = zsb + squishOffset;
		double wb = wsb + squishOffset;

		//Compute simplectic honeycomb coordinates relative to rhombo-hypercube origin.
		double xins = xs - xsb;
		double yins = ys - ysb;
		double zins = zs - zsb;
		double wins = ws - wsb;

		//Sum those together to get a value that determines which region we're in.
		double inSum = xins + yins + zins + wins;

		//Positions relative to origin point.
		double dx0 = x - xb;
		double dy0 = y - yb;
		double dz0 = z - zb;
		double dw0 = w - wb;

		//We'll be defining these inside the next block and using them afterwards.
		double dx_ext0, dy_ext0, dz_ext0, dw_ext0;
		double dx_ext1, dy_ext1, dz_ext1, dw_ext1;
		double dx_ext2, dy_ext2, dz_ext2, dw_ext2;
		int xsv_ext0, ysv_ext0, zsv_ext0, wsv_ext0;
		int xsv_ext1, ysv_ext1, zsv_ext1, wsv_ext1;
		int xsv_ext2, ysv_ext2, zsv_ext2, wsv_ext2;

		double value = 0;
		if (inSum <= 1)
		{ //We're inside the pentachoron (4-Simplex) at (0,0,0,0)

	//Determine which two of (0,0,0,1), (0,0,1,0), (0,1,0,0), (1,0,0,0) are closest.
			char aPoint = 0x01;
			double aScore = xins;
			char bPoint = 0x02;
			double bScore = yins;
			if (aScore >= bScore && zins > bScore)
			{
				bScore = zins;
				bPoint = 0x04;
			}
			else if (aScore < bScore && zins > aScore)
			{
				aScore = zins;
				aPoint = 0x04;
			}
			if (aScore >= bScore && wins > bScore)
			{
				bScore = wins;
				bPoint = 0x08;
			}
			else if (aScore < bScore && wins > aScore)
			{
				aScore = wins;
				aPoint = 0x08;
			}

			//Now we determine the three lattice points not part of the pentachoron that may contribute.
			//This depends on the closest two pentachoron vertices, including (0,0,0,0)
			double uins = 1 - inSum;
			if (uins > aScore || uins > bScore)
			{ //(0,0,0,0) is one of the closest two pentachoron vertices.
				char c = (bScore > aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.
				if ((c & 0x01) == 0)
				{
					xsv_ext0 = xsb - 1;
					xsv_ext1 = xsv_ext2 = xsb;
					dx_ext0 = dx0 + 1;
					dx_ext1 = dx_ext2 = dx0;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb + 1;
					dx_ext0 = dx_ext1 = dx_ext2 = dx0 - 1;
				}

				if ((c & 0x02) == 0)
				{
					ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
					dy_ext0 = dy_ext1 = dy_ext2 = dy0;
					if ((c & 0x01) == 0x01)
					{
						ysv_ext0 -= 1;
						dy_ext0 += 1;
					}
					else
					{
						ysv_ext1 -= 1;
						dy_ext1 += 1;
					}
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
					dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 1;
				}

				if ((c & 0x04) == 0)
				{
					zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
					dz_ext0 = dz_ext1 = dz_ext2 = dz0;
					if ((c & 0x03) != 0)
					{
						if ((c & 0x03) == 0x03)
						{
							zsv_ext0 -= 1;
							dz_ext0 += 1;
						}
						else
						{
							zsv_ext1 -= 1;
							dz_ext1 += 1;
						}
					}
					else
					{
						zsv_ext2 -= 1;
						dz_ext2 += 1;
					}
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
					dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 1;
				}

				if ((c & 0x08) == 0)
				{
					wsv_ext0 = wsv_ext1 = wsb;
					wsv_ext2 = wsb - 1;
					dw_ext0 = dw_ext1 = dw0;
					dw_ext2 = dw0 + 1;
				}
				else
				{
					wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb + 1;
					dw_ext0 = dw_ext1 = dw_ext2 = dw0 - 1;
				}
			}
			else
			{ //(0,0,0,0) is not one of the closest two pentachoron vertices.
				char c = static_cast<char>(aPoint | bPoint); //Our three extra vertices are determined by the closest two.

				if ((c & 0x01) == 0)
				{
					xsv_ext0 = xsv_ext2 = xsb;
					xsv_ext1 = xsb - 1;
					dx_ext0 = dx0 - 2 * m_squish4d;
					dx_ext1 = dx0 + 1 - m_squish4d;
					dx_ext2 = dx0 - m_squish4d;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb + 1;
					dx_ext0 = dx0 - 1 - 2 * m_squish4d;
					dx_ext1 = dx_ext2 = dx0 - 1 - m_squish4d;
				}

				if ((c & 0x02) == 0)
				{
					ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
					dy_ext0 = dy0 - 2 * m_squish4d;
					dy_ext1 = dy_ext2 = dy0 - m_squish4d;
					if ((c & 0x01) == 0x01)
					{
						ysv_ext1 -= 1;
						dy_ext1 += 1;
					}
					else
					{
						ysv_ext2 -= 1;
						dy_ext2 += 1;
					}
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
					dy_ext0 = dy0 - 1 - 2 * m_squish4d;
					dy_ext1 = dy_ext2 = dy0 - 1 - m_squish4d;
				}

				if ((c & 0x04) == 0)
				{
					zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
					dz_ext0 = dz0 - 2 * m_squish4d;
					dz_ext1 = dz_ext2 = dz0 - m_squish4d;
					if ((c & 0x03) == 0x03)
					{
						zsv_ext1 -= 1;
						dz_ext1 += 1;
					}
					else
					{
						zsv_ext2 -= 1;
						dz_ext2 += 1;
					}
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
					dz_ext0 = dz0 - 1 - 2 * m_squish4d;
					dz_ext1 = dz_ext2 = dz0 - 1 - m_squish4d;
				}

				if ((c & 0x08) == 0)
				{
					wsv_ext0 = wsv_ext1 = wsb;
					wsv_ext2 = wsb - 1;
					dw_ext0 = dw0 - 2 * m_squish4d;
					dw_ext1 = dw0 - m_squish4d;
					dw_ext2 = dw0 + 1 - m_squish4d;
				}
				else
				{
					wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb + 1;
					dw_ext0 = dw0 - 1 - 2 * m_squish4d;
					dw_ext1 = dw_ext2 = dw0 - 1 - m_squish4d;
				}
			}

			//Contribution (0,0,0,0)
			double attn0 = 2 - dx0 * dx0 - dy0 * dy0 - dz0 * dz0 - dw0 * dw0;
			if (attn0 > 0)
			{
				attn0 *= attn0;
				value += attn0 * attn0 * extrapolate(xsb + 0, ysb + 0, zsb + 0, wsb + 0, dx0, dy0, dz0, dw0);
			}

			//Contribution (1,0,0,0)
			double dx1 = dx0 - 1 - m_squish4d;
			double dy1 = dy0 - 0 - m_squish4d;
			double dz1 = dz0 - 0 - m_squish4d;
			double dw1 = dw0 - 0 - m_squish4d;
			double attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1 - dw1 * dw1;
			if (attn1 > 0)
			{
				attn1 *= attn1;
				value += attn1 * attn1 * extrapolate(xsb + 1, ysb + 0, zsb + 0, wsb + 0, dx1, dy1, dz1, dw1);
			}

			//Contribution (0,1,0,0)
			double dx2 = dx0 - 0 - m_squish4d;
			double dy2 = dy0 - 1 - m_squish4d;
			double dz2 = dz1;
			double dw2 = dw1;
			double attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2 - dw2 * dw2;
			if (attn2 > 0)
			{
				attn2 *= attn2;
				value += attn2 * attn2 * extrapolate(xsb + 0, ysb + 1, zsb + 0, wsb + 0, dx2, dy2, dz2, dw2);
			}

			//Contribution (0,0,1,0)
			double dx3 = dx2;
			double dy3 = dy1;
			double dz3 = dz0 - 1 - m_squish4d;
			double dw3 = dw1;
			double attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3 - dw3 * dw3;
			if (attn3 > 0)
			{
				attn3 *= attn3;
				value += attn3 * attn3 * extrapolate(xsb + 0, ysb + 0, zsb + 1, wsb + 0, dx3, dy3, dz3, dw3);
			}

			//Contribution (0,0,0,1)
			double dx4 = dx2;
			double dy4 = dy1;
			double dz4 = dz1;
			double dw4 = dw0 - 1 - m_squish4d;
			double attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4 - dw4 * dw4;
			if (attn4 > 0)
			{
				attn4 *= attn4;
				value += attn4 * attn4 * extrapolate(xsb + 0, ysb + 0, zsb + 0, wsb + 1, dx4, dy4, dz4, dw4);
			}
		}
		else if (inSum >= 3)
		{ //We're inside the pentachoron (4-Simplex) at (1,1,1,1)
	//Determine which two of (1,1,1,0), (1,1,0,1), (1,0,1,1), (0,1,1,1) are closest.
			char aPoint = 0x0E;
			double aScore = xins;
			char bPoint = 0x0D;
			double bScore = yins;
			if (aScore <= bScore && zins < bScore)
			{
				bScore = zins;
				bPoint = 0x0B;
			}
			else if (aScore > bScore && zins < aScore)
			{
				aScore = zins;
				aPoint = 0x0B;
			}
			if (aScore <= bScore && wins < bScore)
			{
				bScore = wins;
				bPoint = 0x07;
			}
			else if (aScore > bScore && wins < aScore)
			{
				aScore = wins;
				aPoint = 0x07;
			}

			//Now we determine the three lattice points not part of the pentachoron that may contribute.
			//This depends on the closest two pentachoron vertices, including (0,0,0,0)
			double uins = 4 - inSum;
			if (uins < aScore || uins < bScore)
			{ //(1,1,1,1) is one of the closest two pentachoron vertices.
				char c = (bScore < aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.

				if ((c & 0x01) != 0)
				{
					xsv_ext0 = xsb + 2;
					xsv_ext1 = xsv_ext2 = xsb + 1;
					dx_ext0 = dx0 - 2 - 4 * m_squish4d;
					dx_ext1 = dx_ext2 = dx0 - 1 - 4 * m_squish4d;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb;
					dx_ext0 = dx_ext1 = dx_ext2 = dx0 - 4 * m_squish4d;
				}

				if ((c & 0x02) != 0)
				{
					ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
					dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 1 - 4 * m_squish4d;
					if ((c & 0x01) != 0)
					{
						ysv_ext1 += 1;
						dy_ext1 -= 1;
					}
					else
					{
						ysv_ext0 += 1;
						dy_ext0 -= 1;
					}
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
					dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 4 * m_squish4d;
				}

				if ((c & 0x04) != 0)
				{
					zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
					dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 1 - 4 * m_squish4d;
					if ((c & 0x03) != 0x03)
					{
						if ((c & 0x03) == 0)
						{
							zsv_ext0 += 1;
							dz_ext0 -= 1;
						}
						else
						{
							zsv_ext1 += 1;
							dz_ext1 -= 1;
						}
					}
					else
					{
						zsv_ext2 += 1;
						dz_ext2 -= 1;
					}
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
					dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 4 * m_squish4d;
				}

				if ((c & 0x08) != 0)
				{
					wsv_ext0 = wsv_ext1 = wsb + 1;
					wsv_ext2 = wsb + 2;
					dw_ext0 = dw_ext1 = dw0 - 1 - 4 * m_squish4d;
					dw_ext2 = dw0 - 2 - 4 * m_squish4d;
				}
				else
				{
					wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb;
					dw_ext0 = dw_ext1 = dw_ext2 = dw0 - 4 * m_squish4d;
				}
			}
			else
			{ //(1,1,1,1) is not one of the closest two pentachoron vertices.
				char c = static_cast<char>(aPoint & bPoint); //Our three extra vertices are determined by the closest two.

				if ((c & 0x01) != 0)
				{
					xsv_ext0 = xsv_ext2 = xsb + 1;
					xsv_ext1 = xsb + 2;
					dx_ext0 = dx0 - 1 - 2 * m_squish4d;
					dx_ext1 = dx0 - 2 - 3 * m_squish4d;
					dx_ext2 = dx0 - 1 - 3 * m_squish4d;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb;
					dx_ext0 = dx0 - 2 * m_squish4d;
					dx_ext1 = dx_ext2 = dx0 - 3 * m_squish4d;
				}

				if ((c & 0x02) != 0)
				{
					ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
					dy_ext0 = dy0 - 1 - 2 * m_squish4d;
					dy_ext1 = dy_ext2 = dy0 - 1 - 3 * m_squish4d;
					if ((c & 0x01) != 0)
					{
						ysv_ext2 += 1;
						dy_ext2 -= 1;
					}
					else
					{
						ysv_ext1 += 1;
						dy_ext1 -= 1;
					}
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
					dy_ext0 = dy0 - 2 * m_squish4d;
					dy_ext1 = dy_ext2 = dy0 - 3 * m_squish4d;
				}

				if ((c & 0x04) != 0)
				{
					zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
					dz_ext0 = dz0 - 1 - 2 * m_squish4d;
					dz_ext1 = dz_ext2 = dz0 - 1 - 3 * m_squish4d;
					if ((c & 0x03) != 0)
					{
						zsv_ext2 += 1;
						dz_ext2 -= 1;
					}
					else
					{
						zsv_ext1 += 1;
						dz_ext1 -= 1;
					}
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
					dz_ext0 = dz0 - 2 * m_squish4d;
					dz_ext1 = dz_ext2 = dz0 - 3 * m_squish4d;
				}

				if ((c & 0x08) != 0)
				{
					wsv_ext0 = wsv_ext1 = wsb + 1;
					wsv_ext2 = wsb + 2;
					dw_ext0 = dw0 - 1 - 2 * m_squish4d;
					dw_ext1 = dw0 - 1 - 3 * m_squish4d;
					dw_ext2 = dw0 - 2 - 3 * m_squish4d;
				}
				else
				{
					wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb;
					dw_ext0 = dw0 - 2 * m_squish4d;
					dw_ext1 = dw_ext2 = dw0 - 3 * m_squish4d;
				}
			}

			//Contribution (1,1,1,0)
			double dx4 = dx0 - 1 - 3 * m_squish4d;
			double dy4 = dy0 - 1 - 3 * m_squish4d;
			double dz4 = dz0 - 1 - 3 * m_squish4d;
			double dw4 = dw0 - 3 * m_squish4d;
			double attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4 - dw4 * dw4;
			if (attn4 > 0)
			{
				attn4 *= attn4;
				value += attn4 * attn4 * extrapolate(xsb + 1, ysb + 1, zsb + 1, wsb + 0, dx4, dy4, dz4, dw4);
			}

			//Contribution (1,1,0,1)
			double dx3 = dx4;
			double dy3 = dy4;
			double dz3 = dz0 - 3 * m_squish4d;
			double dw3 = dw0 - 1 - 3 * m_squish4d;
			double attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3 - dw3 * dw3;
			if (attn3 > 0)
			{
				attn3 *= attn3;
				value += attn3 * attn3 * extrapolate(xsb + 1, ysb + 1, zsb + 0, wsb + 1, dx3, dy3, dz3, dw3);
			}

			//Contribution (1,0,1,1)
			double dx2 = dx4;
			double dy2 = dy0 - 3 * m_squish4d;
			double dz2 = dz4;
			double dw2 = dw3;
			double attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2 - dw2 * dw2;
			if (attn2 > 0)
			{
				attn2 *= attn2;
				value += attn2 * attn2 * extrapolate(xsb + 1, ysb + 0, zsb + 1, wsb + 1, dx2, dy2, dz2, dw2);
			}

			//Contribution (0,1,1,1)
			double dx1 = dx0 - 3 * m_squish4d;
			double dz1 = dz4;
			double dy1 = dy4;
			double dw1 = dw3;
			double attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1 - dw1 * dw1;
			if (attn1 > 0)
			{
				attn1 *= attn1;
				value += attn1 * attn1 * extrapolate(xsb + 0, ysb + 1, zsb + 1, wsb + 1, dx1, dy1, dz1, dw1);
			}

			//Contribution (1,1,1,1)
			dx0 = dx0 - 1 - 4 * m_squish4d;
			dy0 = dy0 - 1 - 4 * m_squish4d;
			dz0 = dz0 - 1 - 4 * m_squish4d;
			dw0 = dw0 - 1 - 4 * m_squish4d;
			double attn0 = 2 - dx0 * dx0 - dy0 * dy0 - dz0 * dz0 - dw0 * dw0;
			if (attn0 > 0)
			{
				attn0 *= attn0;
				value += attn0 * attn0 * extrapolate(xsb + 1, ysb + 1, zsb + 1, wsb + 1, dx0, dy0, dz0, dw0);
			}
		}
		else if (inSum <= 2)
		{ //We're inside the first dispentachoron (Rectified 4-Simplex)
			double aScore;
			char aPoint;
			bool aIsBiggerSide = true;
			double bScore;
			char bPoint;
			bool bIsBiggerSide = true;

			//Decide between (1,1,0,0) and (0,0,1,1)
			if (xins + yins > zins + wins)
			{
				aScore = xins + yins;
				aPoint = 0x03;
			}
			else
			{
				aScore = zins + wins;
				aPoint = 0x0C;
			}

			//Decide between (1,0,1,0) and (0,1,0,1)
			if (xins + zins > yins + wins)
			{
				bScore = xins + zins;
				bPoint = 0x05;
			}
			else
			{
				bScore = yins + wins;
				bPoint = 0x0A;
			}

			//Closer between (1,0,0,1) and (0,1,1,0) will replace the further of a and b, if closer.
			if (xins + wins > yins + zins)
			{
				double score = xins + wins;
				if (aScore >= bScore && score > bScore)
				{
					bScore = score;
					bPoint = 0x09;
				}
				else if (aScore < bScore && score > aScore)
				{
					aScore = score;
					aPoint = 0x09;
				}
			}
			else
			{
				double score = yins + zins;
				if (aScore >= bScore && score > bScore)
				{
					bScore = score;
					bPoint = 0x06;
				}
				else if (aScore < bScore && score > aScore)
				{
					aScore = score;
					aPoint = 0x06;
				}
			}

			//Decide if (1,0,0,0) is closer.
			double p1 = 2 - inSum + xins;
			if (aScore >= bScore && p1 > bScore)
			{
				bScore = p1;
				bPoint = 0x01;
				bIsBiggerSide = false;
			}
			else if (aScore < bScore && p1 > aScore)
			{
				aScore = p1;
				aPoint = 0x01;
				aIsBiggerSide = false;
			}

			//Decide if (0,1,0,0) is closer.
			double p2 = 2 - inSum + yins;
			if (aScore >= bScore && p2 > bScore)
			{
				bScore = p2;
				bPoint = 0x02;
				bIsBiggerSide = false;
			}
			else if (aScore < bScore && p2 > aScore)
			{
				aScore = p2;
				aPoint = 0x02;
				aIsBiggerSide = false;
			}

			//Decide if (0,0,1,0) is closer.
			double p3 = 2 - inSum + zins;
			if (aScore >= bScore && p3 > bScore)
			{
				bScore = p3;
				bPoint = 0x04;
				bIsBiggerSide = false;
			}
			else if (aScore < bScore && p3 > aScore)
			{
				aScore = p3;
				aPoint = 0x04;
				aIsBiggerSide = false;
			}

			//Decide if (0,0,0,1) is closer.
			double p4 = 2 - inSum + wins;
			if (aScore >= bScore && p4 > bScore)
			{
				bScore = p4;
				bPoint = 0x08;
				bIsBiggerSide = false;
			}
			else if (aScore < bScore && p4 > aScore)
			{
				aScore = p4;
				aPoint = 0x08;
				aIsBiggerSide = false;
			}

			//Where each of the two closest points are determines how the extra three vertices are calculated.
			if (aIsBiggerSide == bIsBiggerSide)
			{
				if (aIsBiggerSide)
				{ //Both closest points on the bigger side
					char c1 = static_cast<char>(aPoint | bPoint);
					char c2 = static_cast<char>(aPoint & bPoint);
					if ((c1 & 0x01) == 0)
					{
						xsv_ext0 = xsb;
						xsv_ext1 = xsb - 1;
						dx_ext0 = dx0 - 3 * m_squish4d;
						dx_ext1 = dx0 + 1 - 2 * m_squish4d;
					}
					else
					{
						xsv_ext0 = xsv_ext1 = xsb + 1;
						dx_ext0 = dx0 - 1 - 3 * m_squish4d;
						dx_ext1 = dx0 - 1 - 2 * m_squish4d;
					}

					if ((c1 & 0x02) == 0)
					{
						ysv_ext0 = ysb;
						ysv_ext1 = ysb - 1;
						dy_ext0 = dy0 - 3 * m_squish4d;
						dy_ext1 = dy0 + 1 - 2 * m_squish4d;
					}
					else
					{
						ysv_ext0 = ysv_ext1 = ysb + 1;
						dy_ext0 = dy0 - 1 - 3 * m_squish4d;
						dy_ext1 = dy0 - 1 - 2 * m_squish4d;
					}

					if ((c1 & 0x04) == 0)
					{
						zsv_ext0 = zsb;
						zsv_ext1 = zsb - 1;
						dz_ext0 = dz0 - 3 * m_squish4d;
						dz_ext1 = dz0 + 1 - 2 * m_squish4d;
					}
					else
					{
						zsv_ext0 = zsv_ext1 = zsb + 1;
						dz_ext0 = dz0 - 1 - 3 * m_squish4d;
						dz_ext1 = dz0 - 1 - 2 * m_squish4d;
					}

					if ((c1 & 0x08) == 0)
					{
						wsv_ext0 = wsb;
						wsv_ext1 = wsb - 1;
						dw_ext0 = dw0 - 3 * m_squish4d;
						dw_ext1 = dw0 + 1 - 2 * m_squish4d;
					}
					else
					{
						wsv_ext0 = wsv_ext1 = wsb + 1;
						dw_ext0 = dw0 - 1 - 3 * m_squish4d;
						dw_ext1 = dw0 - 1 - 2 * m_squish4d;
					}

					//One combination is a permutation of (0,0,0,2) based on c2
					xsv_ext2 = xsb;
					ysv_ext2 = ysb;
					zsv_ext2 = zsb;
					wsv_ext2 = wsb;
					dx_ext2 = dx0 - 2 * m_squish4d;
					dy_ext2 = dy0 - 2 * m_squish4d;
					dz_ext2 = dz0 - 2 * m_squish4d;
					dw_ext2 = dw0 - 2 * m_squish4d;
					if ((c2 & 0x01) != 0)
					{
						xsv_ext2 += 2;
						dx_ext2 -= 2;
					}
					else if ((c2 & 0x02) != 0)
					{
						ysv_ext2 += 2;
						dy_ext2 -= 2;
					}
					else if ((c2 & 0x04) != 0)
					{
						zsv_ext2 += 2;
						dz_ext2 -= 2;
					}
					else
					{
						wsv_ext2 += 2;
						dw_ext2 -= 2;
					}

				}
				else
				{ //Both closest points on the smaller side
				   //One of the two extra points is (0,0,0,0)
					xsv_ext2 = xsb;
					ysv_ext2 = ysb;
					zsv_ext2 = zsb;
					wsv_ext2 = wsb;
					dx_ext2 = dx0;
					dy_ext2 = dy0;
					dz_ext2 = dz0;
					dw_ext2 = dw0;

					//Other two points are based on the omitted axes.
					char c = static_cast<char>(aPoint | bPoint);

					if ((c & 0x01) == 0)
					{
						xsv_ext0 = xsb - 1;
						xsv_ext1 = xsb;
						dx_ext0 = dx0 + 1 - m_squish4d;
						dx_ext1 = dx0 - m_squish4d;
					}
					else
					{
						xsv_ext0 = xsv_ext1 = xsb + 1;
						dx_ext0 = dx_ext1 = dx0 - 1 - m_squish4d;
					}

					if ((c & 0x02) == 0)
					{
						ysv_ext0 = ysv_ext1 = ysb;
						dy_ext0 = dy_ext1 = dy0 - m_squish4d;
						if ((c & 0x01) == 0x01)
						{
							ysv_ext0 -= 1;
							dy_ext0 += 1;
						}
						else
						{
							ysv_ext1 -= 1;
							dy_ext1 += 1;
						}
					}
					else
					{
						ysv_ext0 = ysv_ext1 = ysb + 1;
						dy_ext0 = dy_ext1 = dy0 - 1 - m_squish4d;
					}

					if ((c & 0x04) == 0)
					{
						zsv_ext0 = zsv_ext1 = zsb;
						dz_ext0 = dz_ext1 = dz0 - m_squish4d;
						if ((c & 0x03) == 0x03)
						{
							zsv_ext0 -= 1;
							dz_ext0 += 1;
						}
						else
						{
							zsv_ext1 -= 1;
							dz_ext1 += 1;
						}
					}
					else
					{
						zsv_ext0 = zsv_ext1 = zsb + 1;
						dz_ext0 = dz_ext1 = dz0 - 1 - m_squish4d;
					}

					if ((c & 0x08) == 0)
					{
						wsv_ext0 = wsb;
						wsv_ext1 = wsb - 1;
						dw_ext0 = dw0 - m_squish4d;
						dw_ext1 = dw0 + 1 - m_squish4d;
					}
					else
					{
						wsv_ext0 = wsv_ext1 = wsb + 1;
						dw_ext0 = dw_ext1 = dw0 - 1 - m_squish4d;
					}

				}
			}
			else
			{ //One point on each "side"
				char c1, c2;
				if (aIsBiggerSide)
				{
					c1 = aPoint;
					c2 = bPoint;
				}
				else
				{
					c1 = bPoint;
					c2 = aPoint;
				}

				//Two contributions are the bigger-sided point with each 0 replaced with -1.
				if ((c1 & 0x01) == 0)
				{
					xsv_ext0 = xsb - 1;
					xsv_ext1 = xsb;
					dx_ext0 = dx0 + 1 - m_squish4d;
					dx_ext1 = dx0 - m_squish4d;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsb + 1;
					dx_ext0 = dx_ext1 = dx0 - 1 - m_squish4d;
				}

				if ((c1 & 0x02) == 0)
				{
					ysv_ext0 = ysv_ext1 = ysb;
					dy_ext0 = dy_ext1 = dy0 - m_squish4d;
					if ((c1 & 0x01) == 0x01)
					{
						ysv_ext0 -= 1;
						dy_ext0 += 1;
					}
					else
					{
						ysv_ext1 -= 1;
						dy_ext1 += 1;
					}
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysb + 1;
					dy_ext0 = dy_ext1 = dy0 - 1 - m_squish4d;
				}

				if ((c1 & 0x04) == 0)
				{
					zsv_ext0 = zsv_ext1 = zsb;
					dz_ext0 = dz_ext1 = dz0 - m_squish4d;
					if ((c1 & 0x03) == 0x03)
					{
						zsv_ext0 -= 1;
						dz_ext0 += 1;
					}
					else
					{
						zsv_ext1 -= 1;
						dz_ext1 += 1;
					}
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsb + 1;
					dz_ext0 = dz_ext1 = dz0 - 1 - m_squish4d;
				}

				if ((c1 & 0x08) == 0)
				{
					wsv_ext0 = wsb;
					wsv_ext1 = wsb - 1;
					dw_ext0 = dw0 - m_squish4d;
					dw_ext1 = dw0 + 1 - m_squish4d;
				}
				else
				{
					wsv_ext0 = wsv_ext1 = wsb + 1;
					dw_ext0 = dw_ext1 = dw0 - 1 - m_squish4d;
				}

				//One contribution is a permutation of (0,0,0,2) based on the smaller-sided point
				xsv_ext2 = xsb;
				ysv_ext2 = ysb;
				zsv_ext2 = zsb;
				wsv_ext2 = wsb;
				dx_ext2 = dx0 - 2 * m_squish4d;
				dy_ext2 = dy0 - 2 * m_squish4d;
				dz_ext2 = dz0 - 2 * m_squish4d;
				dw_ext2 = dw0 - 2 * m_squish4d;
				if ((c2 & 0x01) != 0)
				{
					xsv_ext2 += 2;
					dx_ext2 -= 2;
				}
				else if ((c2 & 0x02) != 0)
				{
					ysv_ext2 += 2;
					dy_ext2 -= 2;
				}
				else if ((c2 & 0x04) != 0)
				{
					zsv_ext2 += 2;
					dz_ext2 -= 2;
				}
				else
				{
					wsv_ext2 += 2;
					dw_ext2 -= 2;
				}
			}

			//Contribution (1,0,0,0)
			double dx1 = dx0 - 1 - m_squish4d;
			double dy1 = dy0 - 0 - m_squish4d;
			double dz1 = dz0 - 0 - m_squish4d;
			double dw1 = dw0 - 0 - m_squish4d;
			double attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1 - dw1 * dw1;
			if (attn1 > 0)
			{
				attn1 *= attn1;
				value += attn1 * attn1 * extrapolate(xsb + 1, ysb + 0, zsb + 0, wsb + 0, dx1, dy1, dz1, dw1);
			}

			//Contribution (0,1,0,0)
			double dx2 = dx0 - 0 - m_squish4d;
			double dy2 = dy0 - 1 - m_squish4d;
			double dz2 = dz1;
			double dw2 = dw1;
			double attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2 - dw2 * dw2;
			if (attn2 > 0)
			{
				attn2 *= attn2;
				value += attn2 * attn2 * extrapolate(xsb + 0, ysb + 1, zsb + 0, wsb + 0, dx2, dy2, dz2, dw2);
			}

			//Contribution (0,0,1,0)
			double dx3 = dx2;
			double dy3 = dy1;
			double dz3 = dz0 - 1 - m_squish4d;
			double dw3 = dw1;
			double attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3 - dw3 * dw3;
			if (attn3 > 0)
			{
				attn3 *= attn3;
				value += attn3 * attn3 * extrapolate(xsb + 0, ysb + 0, zsb + 1, wsb + 0, dx3, dy3, dz3, dw3);
			}

			//Contribution (0,0,0,1)
			double dx4 = dx2;
			double dy4 = dy1;
			double dz4 = dz1;
			double dw4 = dw0 - 1 - m_squish4d;
			double attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4 - dw4 * dw4;
			if (attn4 > 0)
			{
				attn4 *= attn4;
				value += attn4 * attn4 * extrapolate(xsb + 0, ysb + 0, zsb + 0, wsb + 1, dx4, dy4, dz4, dw4);
			}

			//Contribution (1,1,0,0)
			double dx5 = dx0 - 1 - 2 * m_squish4d;
			double dy5 = dy0 - 1 - 2 * m_squish4d;
			double dz5 = dz0 - 0 - 2 * m_squish4d;
			double dw5 = dw0 - 0 - 2 * m_squish4d;
			double attn5 = 2 - dx5 * dx5 - dy5 * dy5 - dz5 * dz5 - dw5 * dw5;
			if (attn5 > 0)
			{
				attn5 *= attn5;
				value += attn5 * attn5 * extrapolate(xsb + 1, ysb + 1, zsb + 0, wsb + 0, dx5, dy5, dz5, dw5);
			}

			//Contribution (1,0,1,0)
			double dx6 = dx0 - 1 - 2 * m_squish4d;
			double dy6 = dy0 - 0 - 2 * m_squish4d;
			double dz6 = dz0 - 1 - 2 * m_squish4d;
			double dw6 = dw0 - 0 - 2 * m_squish4d;
			double attn6 = 2 - dx6 * dx6 - dy6 * dy6 - dz6 * dz6 - dw6 * dw6;
			if (attn6 > 0)
			{
				attn6 *= attn6;
				value += attn6 * attn6 * extrapolate(xsb + 1, ysb + 0, zsb + 1, wsb + 0, dx6, dy6, dz6, dw6);
			}

			//Contribution (1,0,0,1)
			double dx7 = dx0 - 1 - 2 * m_squish4d;
			double dy7 = dy0 - 0 - 2 * m_squish4d;
			double dz7 = dz0 - 0 - 2 * m_squish4d;
			double dw7 = dw0 - 1 - 2 * m_squish4d;
			double attn7 = 2 - dx7 * dx7 - dy7 * dy7 - dz7 * dz7 - dw7 * dw7;
			if (attn7 > 0)
			{
				attn7 *= attn7;
				value += attn7 * attn7 * extrapolate(xsb + 1, ysb + 0, zsb + 0, wsb + 1, dx7, dy7, dz7, dw7);
			}

			//Contribution (0,1,1,0)
			double dx8 = dx0 - 0 - 2 * m_squish4d;
			double dy8 = dy0 - 1 - 2 * m_squish4d;
			double dz8 = dz0 - 1 - 2 * m_squish4d;
			double dw8 = dw0 - 0 - 2 * m_squish4d;
			double attn8 = 2 - dx8 * dx8 - dy8 * dy8 - dz8 * dz8 - dw8 * dw8;
			if (attn8 > 0)
			{
				attn8 *= attn8;
				value += attn8 * attn8 * extrapolate(xsb + 0, ysb + 1, zsb + 1, wsb + 0, dx8, dy8, dz8, dw8);
			}

			//Contribution (0,1,0,1)
			double dx9 = dx0 - 0 - 2 * m_squish4d;
			double dy9 = dy0 - 1 - 2 * m_squish4d;
			double dz9 = dz0 - 0 - 2 * m_squish4d;
			double dw9 = dw0 - 1 - 2 * m_squish4d;
			double attn9 = 2 - dx9 * dx9 - dy9 * dy9 - dz9 * dz9 - dw9 * dw9;
			if (attn9 > 0)
			{
				attn9 *= attn9;
				value += attn9 * attn9 * extrapolate(xsb + 0, ysb + 1, zsb + 0, wsb + 1, dx9, dy9, dz9, dw9);
			}

			//Contribution (0,0,1,1)
			double dx10 = dx0 - 0 - 2 * m_squish4d;
			double dy10 = dy0 - 0 - 2 * m_squish4d;
			double dz10 = dz0 - 1 - 2 * m_squish4d;
			double dw10 = dw0 - 1 - 2 * m_squish4d;
			double attn10 = 2 - dx10 * dx10 - dy10 * dy10 - dz10 * dz10 - dw10 * dw10;
			if (attn10 > 0)
			{
				attn10 *= attn10;
				value += attn10 * attn10 * extrapolate(xsb + 0, ysb + 0, zsb + 1, wsb + 1, dx10, dy10, dz10, dw10);
			}
		}
		else
		{ //We're inside the second dispentachoron (Rectified 4-Simplex)
			double aScore;
			char aPoint;
			bool aIsBiggerSide = true;
			double bScore;
			char bPoint;
			bool bIsBiggerSide = true;

			//Decide between (0,0,1,1) and (1,1,0,0)
			if (xins + yins < zins + wins)
			{
				aScore = xins + yins;
				aPoint = 0x0C;
			}
			else
			{
				aScore = zins + wins;
				aPoint = 0x03;
			}

			//Decide between (0,1,0,1) and (1,0,1,0)
			if (xins + zins < yins + wins)
			{
				bScore = xins + zins;
				bPoint = 0x0A;
			}
			else
			{
				bScore = yins + wins;
				bPoint = 0x05;
			}

			//Closer between (0,1,1,0) and (1,0,0,1) will replace the further of a and b, if closer.
			if (xins + wins < yins + zins)
			{
				double score = xins + wins;
				if (aScore <= bScore && score < bScore)
				{
					bScore = score;
					bPoint = 0x06;
				}
				else if (aScore > bScore && score < aScore)
				{
					aScore = score;
					aPoint = 0x06;
				}
			}
			else
			{
				double score = yins + zins;
				if (aScore <= bScore && score < bScore)
				{
					bScore = score;
					bPoint = 0x09;
				}
				else if (aScore > bScore && score < aScore)
				{
					aScore = score;
					aPoint = 0x09;
				}
			}

			//Decide if (0,1,1,1) is closer.
			double p1 = 3 - inSum + xins;
			if (aScore <= bScore && p1 < bScore)
			{
				bScore = p1;
				bPoint = 0x0E;
				bIsBiggerSide = false;
			}
			else if (aScore > bScore && p1 < aScore)
			{
				aScore = p1;
				aPoint = 0x0E;
				aIsBiggerSide = false;
			}

			//Decide if (1,0,1,1) is closer.
			double p2 = 3 - inSum + yins;
			if (aScore <= bScore && p2 < bScore)
			{
				bScore = p2;
				bPoint = 0x0D;
				bIsBiggerSide = false;
			}
			else if (aScore > bScore && p2 < aScore)
			{
				aScore = p2;
				aPoint = 0x0D;
				aIsBiggerSide = false;
			}

			//Decide if (1,1,0,1) is closer.
			double p3 = 3 - inSum + zins;
			if (aScore <= bScore && p3 < bScore)
			{
				bScore = p3;
				bPoint = 0x0B;
				bIsBiggerSide = false;
			}
			else if (aScore > bScore && p3 < aScore)
			{
				aScore = p3;
				aPoint = 0x0B;
				aIsBiggerSide = false;
			}

			//Decide if (1,1,1,0) is closer.
			double p4 = 3 - inSum + wins;
			if (aScore <= bScore && p4 < bScore)
			{
				bScore = p4;
				bPoint = 0x07;
				bIsBiggerSide = false;
			}
			else if (aScore > bScore && p4 < aScore)
			{
				aScore = p4;
				aPoint = 0x07;
				aIsBiggerSide = false;
			}

			//Where each of the two closest points are determines how the extra three vertices are calculated.
			if (aIsBiggerSide == bIsBiggerSide)
			{
				if (aIsBiggerSide)
				{ //Both closest points on the bigger side
					char c1 = static_cast<char>(aPoint & bPoint);
					char c2 = static_cast<char>(aPoint | bPoint);

					//Two contributions are permutations of (0,0,0,1) and (0,0,0,2) based on c1
					xsv_ext0 = xsv_ext1 = xsb;
					ysv_ext0 = ysv_ext1 = ysb;
					zsv_ext0 = zsv_ext1 = zsb;
					wsv_ext0 = wsv_ext1 = wsb;
					dx_ext0 = dx0 - m_squish4d;
					dy_ext0 = dy0 - m_squish4d;
					dz_ext0 = dz0 - m_squish4d;
					dw_ext0 = dw0 - m_squish4d;
					dx_ext1 = dx0 - 2 * m_squish4d;
					dy_ext1 = dy0 - 2 * m_squish4d;
					dz_ext1 = dz0 - 2 * m_squish4d;
					dw_ext1 = dw0 - 2 * m_squish4d;
					if ((c1 & 0x01) != 0)
					{
						xsv_ext0 += 1;
						dx_ext0 -= 1;
						xsv_ext1 += 2;
						dx_ext1 -= 2;
					}
					else if ((c1 & 0x02) != 0)
					{
						ysv_ext0 += 1;
						dy_ext0 -= 1;
						ysv_ext1 += 2;
						dy_ext1 -= 2;
					}
					else if ((c1 & 0x04) != 0)
					{
						zsv_ext0 += 1;
						dz_ext0 -= 1;
						zsv_ext1 += 2;
						dz_ext1 -= 2;
					}
					else
					{
						wsv_ext0 += 1;
						dw_ext0 -= 1;
						wsv_ext1 += 2;
						dw_ext1 -= 2;
					}

					//One contribution is a permutation of (1,1,1,-1) based on c2
					xsv_ext2 = xsb + 1;
					ysv_ext2 = ysb + 1;
					zsv_ext2 = zsb + 1;
					wsv_ext2 = wsb + 1;
					dx_ext2 = dx0 - 1 - 2 * m_squish4d;
					dy_ext2 = dy0 - 1 - 2 * m_squish4d;
					dz_ext2 = dz0 - 1 - 2 * m_squish4d;
					dw_ext2 = dw0 - 1 - 2 * m_squish4d;
					if ((c2 & 0x01) == 0)
					{
						xsv_ext2 -= 2;
						dx_ext2 += 2;
					}
					else if ((c2 & 0x02) == 0)
					{
						ysv_ext2 -= 2;
						dy_ext2 += 2;
					}
					else if ((c2 & 0x04) == 0)
					{
						zsv_ext2 -= 2;
						dz_ext2 += 2;
					}
					else
					{
						wsv_ext2 -= 2;
						dw_ext2 += 2;
					}
				}
				else
				{ //Both closest points on the smaller side
				   //One of the two extra points is (1,1,1,1)
					xsv_ext2 = xsb + 1;
					ysv_ext2 = ysb + 1;
					zsv_ext2 = zsb + 1;
					wsv_ext2 = wsb + 1;
					dx_ext2 = dx0 - 1 - 4 * m_squish4d;
					dy_ext2 = dy0 - 1 - 4 * m_squish4d;
					dz_ext2 = dz0 - 1 - 4 * m_squish4d;
					dw_ext2 = dw0 - 1 - 4 * m_squish4d;

					//Other two points are based on the shared axes.
					char c = static_cast<char>(aPoint & bPoint);

					if ((c & 0x01) != 0)
					{
						xsv_ext0 = xsb + 2;
						xsv_ext1 = xsb + 1;
						dx_ext0 = dx0 - 2 - 3 * m_squish4d;
						dx_ext1 = dx0 - 1 - 3 * m_squish4d;
					}
					else
					{
						xsv_ext0 = xsv_ext1 = xsb;
						dx_ext0 = dx_ext1 = dx0 - 3 * m_squish4d;
					}

					if ((c & 0x02) != 0)
					{
						ysv_ext0 = ysv_ext1 = ysb + 1;
						dy_ext0 = dy_ext1 = dy0 - 1 - 3 * m_squish4d;
						if ((c & 0x01) == 0)
						{
							ysv_ext0 += 1;
							dy_ext0 -= 1;
						}
						else
						{
							ysv_ext1 += 1;
							dy_ext1 -= 1;
						}
					}
					else
					{
						ysv_ext0 = ysv_ext1 = ysb;
						dy_ext0 = dy_ext1 = dy0 - 3 * m_squish4d;
					}

					if ((c & 0x04) != 0)
					{
						zsv_ext0 = zsv_ext1 = zsb + 1;
						dz_ext0 = dz_ext1 = dz0 - 1 - 3 * m_squish4d;
						if ((c & 0x03) == 0)
						{
							zsv_ext0 += 1;
							dz_ext0 -= 1;
						}
						else
						{
							zsv_ext1 += 1;
							dz_ext1 -= 1;
						}
					}
					else
					{
						zsv_ext0 = zsv_ext1 = zsb;
						dz_ext0 = dz_ext1 = dz0 - 3 * m_squish4d;
					}

					if ((c & 0x08) != 0)
					{
						wsv_ext0 = wsb + 1;
						wsv_ext1 = wsb + 2;
						dw_ext0 = dw0 - 1 - 3 * m_squish4d;
						dw_ext1 = dw0 - 2 - 3 * m_squish4d;
					}
					else
					{
						wsv_ext0 = wsv_ext1 = wsb;
						dw_ext0 = dw_ext1 = dw0 - 3 * m_squish4d;
					}
				}
			}
			else
			{ //One point on each "side"
				char c1, c2;
				if (aIsBiggerSide)
				{
					c1 = aPoint;
					c2 = bPoint;
				}
				else
				{
					c1 = bPoint;
					c2 = aPoint;
				}

				//Two contributions are the bigger-sided point with each 1 replaced with 2.
				if ((c1 & 0x01) != 0)
				{
					xsv_ext0 = xsb + 2;
					xsv_ext1 = xsb + 1;
					dx_ext0 = dx0 - 2 - 3 * m_squish4d;
					dx_ext1 = dx0 - 1 - 3 * m_squish4d;
				}
				else
				{
					xsv_ext0 = xsv_ext1 = xsb;
					dx_ext0 = dx_ext1 = dx0 - 3 * m_squish4d;
				}

				if ((c1 & 0x02) != 0)
				{
					ysv_ext0 = ysv_ext1 = ysb + 1;
					dy_ext0 = dy_ext1 = dy0 - 1 - 3 * m_squish4d;
					if ((c1 & 0x01) == 0)
					{
						ysv_ext0 += 1;
						dy_ext0 -= 1;
					}
					else
					{
						ysv_ext1 += 1;
						dy_ext1 -= 1;
					}
				}
				else
				{
					ysv_ext0 = ysv_ext1 = ysb;
					dy_ext0 = dy_ext1 = dy0 - 3 * m_squish4d;
				}

				if ((c1 & 0x04) != 0)
				{
					zsv_ext0 = zsv_ext1 = zsb + 1;
					dz_ext0 = dz_ext1 = dz0 - 1 - 3 * m_squish4d;
					if ((c1 & 0x03) == 0)
					{
						zsv_ext0 += 1;
						dz_ext0 -= 1;
					}
					else
					{
						zsv_ext1 += 1;
						dz_ext1 -= 1;
					}
				}
				else
				{
					zsv_ext0 = zsv_ext1 = zsb;
					dz_ext0 = dz_ext1 = dz0 - 3 * m_squish4d;
				}

				if ((c1 & 0x08) != 0)
				{
					wsv_ext0 = wsb + 1;
					wsv_ext1 = wsb + 2;
					dw_ext0 = dw0 - 1 - 3 * m_squish4d;
					dw_ext1 = dw0 - 2 - 3 * m_squish4d;
				}
				else
				{
					wsv_ext0 = wsv_ext1 = wsb;
					dw_ext0 = dw_ext1 = dw0 - 3 * m_squish4d;
				}

				//One contribution is a permutation of (1,1,1,-1) based on the smaller-sided point
				xsv_ext2 = xsb + 1;
				ysv_ext2 = ysb + 1;
				zsv_ext2 = zsb + 1;
				wsv_ext2 = wsb + 1;
				dx_ext2 = dx0 - 1 - 2 * m_squish4d;
				dy_ext2 = dy0 - 1 - 2 * m_squish4d;
				dz_ext2 = dz0 - 1 - 2 * m_squish4d;
				dw_ext2 = dw0 - 1 - 2 * m_squish4d;
				if ((c2 & 0x01) == 0)
				{
					xsv_ext2 -= 2;
					dx_ext2 += 2;
				}
				else if ((c2 & 0x02) == 0)
				{
					ysv_ext2 -= 2;
					dy_ext2 += 2;
				}
				else if ((c2 & 0x04) == 0)
				{
					zsv_ext2 -= 2;
					dz_ext2 += 2;
				}
				else
				{
					wsv_ext2 -= 2;
					dw_ext2 += 2;
				}
			}

			//Contribution (1,1,1,0)
			double dx4 = dx0 - 1 - 3 * m_squish4d;
			double dy4 = dy0 - 1 - 3 * m_squish4d;
			double dz4 = dz0 - 1 - 3 * m_squish4d;
			double dw4 = dw0 - 3 * m_squish4d;
			double attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4 - dw4 * dw4;
			if (attn4 > 0)
			{
				attn4 *= attn4;
				value += attn4 * attn4 * extrapolate(xsb + 1, ysb + 1, zsb + 1, wsb + 0, dx4, dy4, dz4, dw4);
			}

			//Contribution (1,1,0,1)
			double dx3 = dx4;
			double dy3 = dy4;
			double dz3 = dz0 - 3 * m_squish4d;
			double dw3 = dw0 - 1 - 3 * m_squish4d;
			double attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3 - dw3 * dw3;
			if (attn3 > 0)
			{
				attn3 *= attn3;
				value += attn3 * attn3 * extrapolate(xsb + 1, ysb + 1, zsb + 0, wsb + 1, dx3, dy3, dz3, dw3);
			}

			//Contribution (1,0,1,1)
			double dx2 = dx4;
			double dy2 = dy0 - 3 * m_squish4d;
			double dz2 = dz4;
			double dw2 = dw3;
			double attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2 - dw2 * dw2;
			if (attn2 > 0)
			{
				attn2 *= attn2;
				value += attn2 * attn2 * extrapolate(xsb + 1, ysb + 0, zsb + 1, wsb + 1, dx2, dy2, dz2, dw2);
			}

			//Contribution (0,1,1,1)
			double dx1 = dx0 - 3 * m_squish4d;
			double dz1 = dz4;
			double dy1 = dy4;
			double dw1 = dw3;
			double attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1 - dw1 * dw1;
			if (attn1 > 0)
			{
				attn1 *= attn1;
				value += attn1 * attn1 * extrapolate(xsb + 0, ysb + 1, zsb + 1, wsb + 1, dx1, dy1, dz1, dw1);
			}

			//Contribution (1,1,0,0)
			double dx5 = dx0 - 1 - 2 * m_squish4d;
			double dy5 = dy0 - 1 - 2 * m_squish4d;
			double dz5 = dz0 - 0 - 2 * m_squish4d;
			double dw5 = dw0 - 0 - 2 * m_squish4d;
			double attn5 = 2 - dx5 * dx5 - dy5 * dy5 - dz5 * dz5 - dw5 * dw5;
			if (attn5 > 0)
			{
				attn5 *= attn5;
				value += attn5 * attn5 * extrapolate(xsb + 1, ysb + 1, zsb + 0, wsb + 0, dx5, dy5, dz5, dw5);
			}

			//Contribution (1,0,1,0)
			double dx6 = dx0 - 1 - 2 * m_squish4d;
			double dy6 = dy0 - 0 - 2 * m_squish4d;
			double dz6 = dz0 - 1 - 2 * m_squish4d;
			double dw6 = dw0 - 0 - 2 * m_squish4d;
			double attn6 = 2 - dx6 * dx6 - dy6 * dy6 - dz6 * dz6 - dw6 * dw6;
			if (attn6 > 0)
			{
				attn6 *= attn6;
				value += attn6 * attn6 * extrapolate(xsb + 1, ysb + 0, zsb + 1, wsb + 0, dx6, dy6, dz6, dw6);
			}

			//Contribution (1,0,0,1)
			double dx7 = dx0 - 1 - 2 * m_squish4d;
			double dy7 = dy0 - 0 - 2 * m_squish4d;
			double dz7 = dz0 - 0 - 2 * m_squish4d;
			double dw7 = dw0 - 1 - 2 * m_squish4d;
			double attn7 = 2 - dx7 * dx7 - dy7 * dy7 - dz7 * dz7 - dw7 * dw7;
			if (attn7 > 0)
			{
				attn7 *= attn7;
				value += attn7 * attn7 * extrapolate(xsb + 1, ysb + 0, zsb + 0, wsb + 1, dx7, dy7, dz7, dw7);
			}

			//Contribution (0,1,1,0)
			double dx8 = dx0 - 0 - 2 * m_squish4d;
			double dy8 = dy0 - 1 - 2 * m_squish4d;
			double dz8 = dz0 - 1 - 2 * m_squish4d;
			double dw8 = dw0 - 0 - 2 * m_squish4d;
			double attn8 = 2 - dx8 * dx8 - dy8 * dy8 - dz8 * dz8 - dw8 * dw8;
			if (attn8 > 0)
			{
				attn8 *= attn8;
				value += attn8 * attn8 * extrapolate(xsb + 0, ysb + 1, zsb + 1, wsb + 0, dx8, dy8, dz8, dw8);
			}

			//Contribution (0,1,0,1)
			double dx9 = dx0 - 0 - 2 * m_squish4d;
			double dy9 = dy0 - 1 - 2 * m_squish4d;
			double dz9 = dz0 - 0 - 2 * m_squish4d;
			double dw9 = dw0 - 1 - 2 * m_squish4d;
			double attn9 = 2 - dx9 * dx9 - dy9 * dy9 - dz9 * dz9 - dw9 * dw9;
			if (attn9 > 0)
			{
				attn9 *= attn9;
				value += attn9 * attn9 * extrapolate(xsb + 0, ysb + 1, zsb + 0, wsb + 1, dx9, dy9, dz9, dw9);
			}

			//Contribution (0,0,1,1)
			double dx10 = dx0 - 0 - 2 * m_squish4d;
			double dy10 = dy0 - 0 - 2 * m_squish4d;
			double dz10 = dz0 - 1 - 2 * m_squish4d;
			double dw10 = dw0 - 1 - 2 * m_squish4d;
			double attn10 = 2 - dx10 * dx10 - dy10 * dy10 - dz10 * dz10 - dw10 * dw10;
			if (attn10 > 0)
			{
				attn10 *= attn10;
				value += attn10 * attn10 * extrapolate(xsb + 0, ysb + 0, zsb + 1, wsb + 1, dx10, dy10, dz10, dw10);
			}
		}

		//First extra vertex
		double attn_ext0 = 2 - dx_ext0 * dx_ext0 - dy_ext0 * dy_ext0 - dz_ext0 * dz_ext0 - dw_ext0 * dw_ext0;
		if (attn_ext0 > 0)
		{
			attn_ext0 *= attn_ext0;
			value += attn_ext0 * attn_ext0 * extrapolate(xsv_ext0, ysv_ext0, zsv_ext0, wsv_ext0, dx_ext0, dy_ext0, dz_ext0, dw_ext0);
		}

		//Second extra vertex
		double attn_ext1 = 2 - dx_ext1 * dx_ext1 - dy_ext1 * dy_ext1 - dz_ext1 * dz_ext1 - dw_ext1 * dw_ext1;
		if (attn_ext1 > 0)
		{
			attn_ext1 *= attn_ext1;
			value += attn_ext1 * attn_ext1 * extrapolate(xsv_ext1, ysv_ext1, zsv_ext1, wsv_ext1, dx_ext1, dy_ext1, dz_ext1, dw_ext1);
		}

		//Third extra vertex
		double attn_ext2 = 2 - dx_ext2 * dx_ext2 - dy_ext2 * dy_ext2 - dz_ext2 * dz_ext2 - dw_ext2 * dw_ext2;
		if (attn_ext2 > 0)
		{
			attn_ext2 *= attn_ext2;
			value += attn_ext2 * attn_ext2 * extrapolate(xsv_ext2, ysv_ext2, zsv_ext2, wsv_ext2, dx_ext2, dy_ext2, dz_ext2, dw_ext2);
		}

		return value / m_norm4d;
	}

	double Noise::extrapolate(int xsb, int ysb, double dx, double dy) const
	{
		int index = m_perm[(m_perm[xsb & 0xFF] + ysb) & 0xFF] & 0x0E;
		return m_gradients2d[index] * dx
			+ m_gradients2d[index + 1] * dy;
	}

	double Noise::extrapolate(int xsb, int ysb, int zsb, double dx, double dy, double dz) const
	{
		int index = m_permGradIndex3d[(m_perm[(m_perm[xsb & 0xFF] + ysb) & 0xFF] + zsb) & 0xFF];
		return m_gradients3d[index] * dx
			+ m_gradients3d[index + 1] * dy
			+ m_gradients3d[index + 2] * dz;
	}

	double Noise::extrapolate(int xsb, int ysb, int zsb, int wsb, double dx, double dy, double dz, double dw) const
	{
		int index = m_perm[(m_perm[(m_perm[(m_perm[xsb & 0xFF] + ysb) & 0xFF] + zsb) & 0xFF] + wsb) & 0xFF] & 0xFC;
		return m_gradients4d[index] * dx
			+ m_gradients4d[index + 1] * dy
			+ m_gradients4d[index + 2] * dz
			+ m_gradients4d[index + 3] * dw;
	}

}

//----------------------------------------------------------------------------------------
//
//	siv::PerlinNoise
//	Perlin noise library for modern C++
//
//	Copyright (C) 2013-2020 Ryo Suzuki <reputeless@gmail.com>
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files(the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions :
//	
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.
//
//----------------------------------------------------------------------------------------

# pragma once
# include <cstdint>
# include <algorithm>
# include <array>
# ifdef __cpp_concepts
# if __has_include(<concepts>)
# include <concepts>
# endif
# endif
# include <iterator>
# include <numeric>
# include <random>
# include <type_traits>

namespace siv
{
# ifdef __cpp_lib_concepts
	template <std::floating_point Float>
# else
	template <class Float>
# endif
	class BasicPerlinNoise
	{
	public:

		using value_type = Float;

	private:

		std::uint8_t p[512];

		static constexpr value_type Fade(value_type t) noexcept
		{
			return t * t * t * (t * (t * 6 - 15) + 10);
		}

		static constexpr value_type Lerp(value_type t, value_type a, value_type b) noexcept
		{
			return a + t * (b - a);
		}

		static constexpr value_type Grad(std::uint8_t hash, value_type x, value_type y, value_type z) noexcept
		{
			const std::uint8_t h = hash & 15;
			const value_type u = h < 8 ? x : y;
			const value_type v = h < 4 ? y : h == 12 || h == 14 ? x : z;
			return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
		}

		static constexpr value_type Weight(std::int32_t octaves) noexcept
		{
			value_type amp = 1;
			value_type value = 0;

			for (std::int32_t i = 0; i < octaves; ++i)
			{
				value += amp;
				amp /= 2;
			}

			return value;
		}

	public:

# if __has_cpp_attribute(nodiscard) >= 201907L
		[[nodiscard]]
# endif
		explicit BasicPerlinNoise(std::uint32_t seed = std::default_random_engine::default_seed)
		{
			reseed(seed);
		}

# ifdef __cpp_lib_concepts
		template <std::uniform_random_bit_generator URNG>
# else
		template <class URNG, std::enable_if_t<!std::is_arithmetic_v<URNG>>* = nullptr>
# endif
# if __has_cpp_attribute(nodiscard) >= 201907L
		[[nodiscard]]
# endif
		explicit BasicPerlinNoise(URNG&& urng)
		{
			reseed(std::forward<URNG>(urng));
		}

		void reseed(std::uint32_t seed)
		{
			for (size_t i = 0; i < 256; ++i)
			{
				p[i] = static_cast<std::uint8_t>(i);
			}

			std::shuffle(std::begin(p), std::begin(p) + 256, std::default_random_engine(seed));

			for (size_t i = 0; i < 256; ++i)
			{
				p[256 + i] = p[i];
			}
		}

# ifdef __cpp_lib_concepts
		template <std::uniform_random_bit_generator URNG>
# else
		template <class URNG, std::enable_if_t<!std::is_arithmetic_v<URNG>>* = nullptr>
# endif
		void reseed(URNG&& urng)
		{
			for (size_t i = 0; i < 256; ++i)
			{
				p[i] = static_cast<std::uint8_t>(i);
			}

			std::shuffle(std::begin(p), std::begin(p) + 256, std::forward<URNG>(urng));

			for (size_t i = 0; i < 256; ++i)
			{
				p[256 + i] = p[i];
			}
		}

		///////////////////////////////////////
		//
		//	Noise [-1, 1]
		//
		value_type noise1D(value_type x) const noexcept
		{
			return noise3D(x, 0, 0);
		}

		value_type noise2D(value_type x, value_type y) const noexcept
		{
			return noise3D(x, y, 0);
		}

		value_type noise3D(value_type x, value_type y, value_type z) const noexcept
		{
			const std::int32_t X = static_cast<std::int32_t>(std::floor(x)) & 255;
			const std::int32_t Y = static_cast<std::int32_t>(std::floor(y)) & 255;
			const std::int32_t Z = static_cast<std::int32_t>(std::floor(z)) & 255;

			x -= std::floor(x);
			y -= std::floor(y);
			z -= std::floor(z);

			const value_type u = Fade(x);
			const value_type v = Fade(y);
			const value_type w = Fade(z);

			const std::int32_t A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
			const std::int32_t B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

			return Lerp(w, Lerp(v, Lerp(u, Grad(p[AA], x, y, z),
				Grad(p[BA], x - 1, y, z)),
				Lerp(u, Grad(p[AB], x, y - 1, z),
					Grad(p[BB], x - 1, y - 1, z))),
				Lerp(v, Lerp(u, Grad(p[AA + 1], x, y, z - 1),
					Grad(p[BA + 1], x - 1, y, z - 1)),
					Lerp(u, Grad(p[AB + 1], x, y - 1, z - 1),
						Grad(p[BB + 1], x - 1, y - 1, z - 1))));
		}

		///////////////////////////////////////
		//
		//	Noise [0, 1]
		//
		value_type noise1D_0_1(value_type x) const noexcept
		{
			return noise1D(x)
				* value_type(0.5) + value_type(0.5);
		}

		value_type noise2D_0_1(value_type x, value_type y) const noexcept
		{
			return noise2D(x, y)
				* value_type(0.5) + value_type(0.5);
		}

		value_type noise3D_0_1(value_type x, value_type y, value_type z) const noexcept
		{
			return noise3D(x, y, z)
				* value_type(0.5) + value_type(0.5);
		}

		///////////////////////////////////////
		//
		//	Accumulated octave noise
		//	* Return value can be outside the range [-1, 1]
		//
		value_type accumulatedOctaveNoise1D(value_type x, std::int32_t octaves) const noexcept
		{
			value_type result = 0;
			value_type amp = 1;

			for (std::int32_t i = 0; i < octaves; ++i)
			{
				result += noise1D(x) * amp;
				x *= 2;
				amp /= 2;
			}

			return result; // unnormalized
		}

		value_type accumulatedOctaveNoise2D(value_type x, value_type y, std::int32_t octaves) const noexcept
		{
			value_type result = 0;
			value_type amp = 1;

			for (std::int32_t i = 0; i < octaves; ++i)
			{
				result += noise2D(x, y) * amp;
				x *= 2;
				y *= 2;
				amp /= 2;
			}

			return result; // unnormalized
		}

		value_type accumulatedOctaveNoise3D(value_type x, value_type y, value_type z, std::int32_t octaves) const noexcept
		{
			value_type result = 0;
			value_type amp = 1;

			for (std::int32_t i = 0; i < octaves; ++i)
			{
				result += noise3D(x, y, z) * amp;
				x *= 2;
				y *= 2;
				z *= 2;
				amp /= 2;
			}

			return result; // unnormalized
		}

		///////////////////////////////////////
		//
		//	Normalized octave noise [-1, 1]
		//
		value_type normalizedOctaveNoise1D(value_type x, std::int32_t octaves) const noexcept
		{
			return accumulatedOctaveNoise1D(x, octaves)
				/ Weight(octaves);
		}

		value_type normalizedOctaveNoise2D(value_type x, value_type y, std::int32_t octaves) const noexcept
		{
			return accumulatedOctaveNoise2D(x, y, octaves)
				/ Weight(octaves);
		}

		value_type normalizedOctaveNoise3D(value_type x, value_type y, value_type z, std::int32_t octaves) const noexcept
		{
			return accumulatedOctaveNoise3D(x, y, z, octaves)
				/ Weight(octaves);
		}

		///////////////////////////////////////
		//
		//	Accumulated octave noise clamped within the range [0, 1]
		//
		value_type accumulatedOctaveNoise1D_0_1(value_type x, std::int32_t octaves) const noexcept
		{
			return std::clamp<value_type>(accumulatedOctaveNoise1D(x, octaves)
				* value_type(0.5) + value_type(0.5), 0, 1);
		}

		value_type accumulatedOctaveNoise2D_0_1(value_type x, value_type y, std::int32_t octaves) const noexcept
		{
			return std::clamp<value_type>(accumulatedOctaveNoise2D(x, y, octaves)
				* value_type(0.5) + value_type(0.5), 0, 1);
		}

		value_type accumulatedOctaveNoise3D_0_1(value_type x, value_type y, value_type z, std::int32_t octaves) const noexcept
		{
			return std::clamp<value_type>(accumulatedOctaveNoise3D(x, y, z, octaves)
				* value_type(0.5) + value_type(0.5), 0, 1);
		}

		///////////////////////////////////////
		//
		//	Normalized octave noise [0, 1]
		//
		value_type normalizedOctaveNoise1D_0_1(value_type x, std::int32_t octaves) const noexcept
		{
			return normalizedOctaveNoise1D(x, octaves)
				* value_type(0.5) + value_type(0.5);
		}

		value_type normalizedOctaveNoise2D_0_1(value_type x, value_type y, std::int32_t octaves) const noexcept
		{
			return normalizedOctaveNoise2D(x, y, octaves)
				* value_type(0.5) + value_type(0.5);
		}

		value_type normalizedOctaveNoise3D_0_1(value_type x, value_type y, value_type z, std::int32_t octaves) const noexcept
		{
			return normalizedOctaveNoise3D(x, y, z, octaves)
				* value_type(0.5) + value_type(0.5);
		}

		///////////////////////////////////////
		//
		//	Serialization
		//
		void serialize(std::array<std::uint8_t, 256>& s) const noexcept
		{
			for (std::size_t i = 0; i < 256; ++i)
			{
				s[i] = p[i];
			}
		}

		void deserialize(const std::array<std::uint8_t, 256>& s) noexcept
		{
			for (std::size_t i = 0; i < 256; ++i)
			{
				p[256 + i] = p[i] = s[i];
			}
		}

		///////////////////////////////////////
		//
		//	Legacy interface
		//
		double noise(double x) const;
		double noise(double x, double y) const;
		double noise(double x, double y, double z) const;

		double noise0_1(double x) const;
		double noise0_1(double x, double y) const;
		double noise0_1(double x, double y, double z) const;

		double octaveNoise(double x, std::int32_t octaves) const;
		double octaveNoise(double x, double y, std::int32_t octaves) const;
		double octaveNoise(double x, double y, double z, std::int32_t octaves) const;

		double octaveNoise0_1(double x, std::int32_t octaves) const;
		double octaveNoise0_1(double x, double y, std::int32_t octaves) const;
		double octaveNoise0_1(double x, double y, double z, std::int32_t octaves) const;
	};

	using PerlinNoise = BasicPerlinNoise<double>;
}











#if USE_IPP
#include <ipp.h>
#include <ippcv.h>
#endif

namespace hise {
using namespace juce;

PostGraphicsRenderer::Data::~Data()
{
#if USE_IPP
	if (pBuffer)
		ippsFree(pBuffer);

	if (pSpec)
		ippsFree(pSpec);
#endif
}

void PostGraphicsRenderer::Data::increaseIfNecessary(int minSize)
{
#if USE_IPP
	if (minSize > bufSize)
	{
		if (pBuffer) ippsFree(pBuffer);
		bufSize = minSize;
		pBuffer = ippsMalloc_8u(minSize);
	}
#endif
}

void PostGraphicsRenderer::Data::createPathImage(int width, int height)
{
	if (pathImage.getWidth() != width || pathImage.getHeight() != height)
	{
		pathImage = Image(Image::SingleChannel, width, height, true);
	}
	else
		pathImage.clear({ 0, 0, width, height });
}

bool PostGraphicsRenderer::Data::initGaussianBlur(int kernelSize, float sigma, int width, int height)
{
#if USE_IPP && JUCE_WINDOWS
	auto thisNumPixels = width * height;

	if (thisNumPixels != numPixels || kernelSize != lastKernelSize)
	{
		lastKernelSize = kernelSize;
		numPixels = thisNumPixels;

		IppStatus status = ippStsNoErr;

		IppiSize srcSize = { width, height };

		int pSpecSize, thisBufferSize;
		withoutAlpha.allocate(numPixels * 3, false);

		status = ippiFilterGaussianGetBufferSize(srcSize, kernelSize, ipp8u, 3, &pSpecSize, &thisBufferSize);

		increaseIfNecessary(thisBufferSize);

		if (pSpec)
			ippsFree(pSpec);

		pSpec = (IppFilterGaussianSpec *)ippMalloc(pSpecSize);

		status = ippiFilterGaussianInit(srcSize, kernelSize, sigma, ippBorderRepl, ipp8u, 3, reinterpret_cast<IppFilterGaussianSpec*>(pSpec), static_cast<Ipp8u*>(pBuffer));

		return true;
	}
#endif

	return false;

}

PostGraphicsRenderer::Data::WithoutAlphaConverter::WithoutAlphaConverter(Data& bf_, Image::BitmapData& bd_) :
	bd(bd_),
	bf(bf_)
{
	for (int i = 0; i < bf.numPixels; i++)
	{
		bf.withoutAlpha[i * 3] = bd.data[i * 4];
		bf.withoutAlpha[i * 3 + 1] = bd.data[i * 4 + 1];
		bf.withoutAlpha[i * 3 + 2] = bd.data[i * 4 + 2];
	}
}

PostGraphicsRenderer::Data::WithoutAlphaConverter::~WithoutAlphaConverter()
{
	for (int i = 0; i < bf.numPixels; i++)
	{
		bd.data[i * 4] = bf.withoutAlpha[i * 3];
		bd.data[i * 4 + 1] = bf.withoutAlpha[i * 3 + 1];
		bd.data[i * 4 + 2] = bf.withoutAlpha[i * 3 + 2];
	}
}

PostGraphicsRenderer::PostGraphicsRenderer(DataStack& stackTouse, Image& image) :
	img(image),
	bd(image, Image::BitmapData::readWrite),
	stack(stackTouse)
{
	
}

void PostGraphicsRenderer::reserveStackOperations(int numOperationsToAllocate)
{
	int numToAdd = numOperationsToAllocate - stack.size();

	while (--numToAdd >= 0)
		stack.add(new Data());
}

void PostGraphicsRenderer::desaturate()
{
	for (int y = 0; y < bd.height; y++)
	{
		for (int x = 0; x < bd.width; x++)
		{
			Pixel p(bd.getPixelPointer(x, y));

			auto sum = (*p.r / 3 + *p.g / 3 + *p.b / 3);
			*p.r = sum;
			*p.g = sum;
			*p.b = sum;
		}
	}
}

void PostGraphicsRenderer::applyMask(Path& path, bool invert /*= false*/, bool scale)
{
	auto& bf = getNextData();

	if (scale)
	{
		Rectangle<float> area(0.0f, 0.0f, bd.width, bd.height);
		PathFactory::scalePath(path, area);
	}

	bf.createPathImage(bd.width, bd.height);

	Graphics g(bf.pathImage);
	g.setColour(Colours::white);
	g.fillPath(path);

	Image::BitmapData pathData(bf.pathImage, Image::BitmapData::readOnly);

	for (int y = 0; y < bd.height; y++)
	{
		for (int x = 0; x < bd.width; x++)
		{
			Pixel p(bd.getPixelPointer(x, y));
			auto ptr = pathData.getPixelPointer(x, y);

			float alpha = (float)*ptr / 255.0f;
			if (invert)
				alpha = 1.0f - alpha;

			*p.r = (uint8)jlimit(0, 255, (int)((float)*p.r * alpha));
			*p.g = (uint8)jlimit(0, 255, (int)((float)*p.g * alpha));
			*p.b = (uint8)jlimit(0, 255, (int)((float)*p.b * alpha));
			*p.a = (uint8)jlimit(0, 255, (int)((float)*p.a * alpha));
		}
	}
}

siv::BasicPerlinNoise<float> perlinNoise;
static int counter = 0;

void PostGraphicsRenderer::addNoise(float noiseAmount)
{
	Random r;

	for (int y = 0; y < bd.height; y++)
	{
		for (int x = 0; x < bd.width; x++)
		{
			Pixel p(bd.getPixelPointer(x, y));

			auto thisNoiseDelta = (r.nextFloat()*2.0f - 1.0f) * noiseAmount;

			auto delta = roundToInt(thisNoiseDelta * 128.0f);

			*p.r = (uint8)jlimit(0, 255, (int)*p.r + delta);
			*p.g = (uint8)jlimit(0, 255, (int)*p.g + delta);
			*p.b = (uint8)jlimit(0, 255, (int)*p.b + delta);
		}
	}
}


static OpenSimplexNoise::Noise osn;

void PostGraphicsRenderer::addPerlinNoise(float freq, float octave, float z, float gain)
{
	auto xDelta = 1.0f / bd.width;
	auto yDelta = 1.0f / bd.height;

	for (int y = 0; y < bd.height; y++)
	{
		for (int x = 0; x < bd.width; x++)
		{
			Pixel p(bd.getPixelPointer(x, y));

			auto thisNoiseDelta = osn.eval(x / freq, y / freq, z);

			//auto thisNoiseDelta = perlinNoise.accumulatedOctaveNoise3D(x / freq, y / freq, z, octave);

			auto delta = roundToInt(gain * thisNoiseDelta * 128.0f);

			*p.r = (uint8)jlimit(0, 255, (int)*p.r + delta);
			*p.g = (uint8)jlimit(0, 255, (int)*p.g + delta);
			*p.b = (uint8)jlimit(0, 255, (int)*p.b + delta);
		}
	}
}

void PostGraphicsRenderer::gaussianBlur(int blur)
{
	
#if USE_IPP && JUCE_WINDOWS
	auto& bf = getNextData();

	blur /= 2;
	int kernelSize = blur * 2 + 1;

	bf.initGaussianBlur(kernelSize, (float)blur, bd.width, bd.height);

	Data::WithoutAlphaConverter wac(bf, bd);

	auto status = ippiFilterGaussianBorder_8u_C3R(wac.getWithoutAlpha(), 3 * bd.width, wac.getWithoutAlpha(), 3 * bd.width, { bd.width, bd.height }, NULL, reinterpret_cast<IppFilterGaussianSpec*>(bf.pSpec), bf.pBuffer);
#endif
}

void PostGraphicsRenderer::boxBlur(int blur)
{
	stackBlur(blur);
	return;

#if USE_IPP
	auto& bf = getNextData();

	IppStatus status = ippStsNoErr;

	IppiSize maskSize = { blur,blur };
	IppiSize srcSize = { bd.width, bd.height };
	Ipp8u* pSrc = bd.data;
	Ipp8u* pDst = bd.data;
	int thisBufSize;

	status = ippiFilterBoxBorderGetBufferSize(srcSize, maskSize, ipp8u, 4, &thisBufSize);

	bf.increaseIfNecessary(thisBufSize);

	if (status >= ippStsNoErr)
		status = ippiFilterBoxBorder_8u_C4R(pSrc, bd.pixelStride * bd.width, pDst, bd.pixelStride * bd.width, srcSize, maskSize, ippBorderRepl, NULL, bf.pBuffer);
#endif
}

void PostGraphicsRenderer::stackBlur(int blur)
{
	static constexpr int DownsamplingFactor = 2;
	static constexpr int NumBytesPerPixel = 4;
	auto f = img.rescaled(img.getWidth() / DownsamplingFactor, img.getHeight() / DownsamplingFactor, Graphics::ResamplingQuality::lowResamplingQuality);

	gin::applyStackBlurARGB(f, blur / DownsamplingFactor);

	juce::Image::BitmapData srcData(f, juce::Image::BitmapData::readOnly);
	juce::Image::BitmapData dstData(img, juce::Image::BitmapData::writeOnly);

	for (int y = 0; y < f.getHeight(); y++)
	{
		auto yDst = y * DownsamplingFactor;
		auto s = srcData.getLinePointer(y);
		

		for (int yd = 0; yd < DownsamplingFactor; yd++)
		{
			auto d = dstData.getLinePointer(yDst + yd);

			for (int x = 0; x < f.getWidth(); x++)
			{
				auto xDst = x * DownsamplingFactor;

				for (int xd = 0; xd < DownsamplingFactor; xd++)
				{
					memcpy(d + (xDst + xd)*NumBytesPerPixel, s + x* NumBytesPerPixel, NumBytesPerPixel);
				}
			}
		}
	}
}

void PostGraphicsRenderer::applyHSL(float h, float s, float l)
{
	gin::applyHueSaturationLightness(img, h, s, l);
}

void PostGraphicsRenderer::applyGamma(float g)
{
	gin::applyGamma(img, g);
}

void PostGraphicsRenderer::applyGradientMap(ColourGradient g)
{
	gin::applyGradientMap(img, g.getColour(0), g.getColour(1));
}

void PostGraphicsRenderer::applySharpness(int delta)
{
	if (delta > 0)
	{
		for (int i = 0; i < delta; i++)
			gin::applySharpen(img);
	}
	else
	{
		for (int i = 0; i < -delta; i++)
			gin::applySoften(img);
	}
}

void PostGraphicsRenderer::applySepia()
{
	gin::applySepia(img);
}

void PostGraphicsRenderer::applyVignette(float amount, float radius, float falloff)
{
	gin::applyVignette(img, amount, radius, falloff);
}

hise::PostGraphicsRenderer::Data& PostGraphicsRenderer::getNextData()
{
	if (isPositiveAndBelow(stackIndex, stack.size()))
	{
		return *stack[stackIndex++];
	}
	else
	{
		if (stack.size() == 0)
		{
			stack.add(new Data());
			return *stack[0];
		}
		else
			return *stack.getLast();
	}
}

PostGraphicsRenderer::Pixel::Pixel(uint8* ptr) :
	a(ptr + 3),
	r(ptr + 2),
	g(ptr + 1),
	b(ptr)
{

}

void ComponentWithPostGraphicsRenderer::paint(Graphics& g)
{
	if (recursive)
		return;

	if (drawOverParent && getParentComponent() != nullptr)
	{
		ScopedValueSetter<bool> vs(recursive, true);
		img = getParentComponent()->createComponentSnapshot(getBoundsInParent());
	}
	else
	{
		if (img.getWidth() != getWidth() || img.getHeight() != getHeight())
		{
			img = Image(Image::ARGB, getWidth(), getHeight(), true);
		}
		else
		{
			img.clear(getLocalBounds());
		}
	}

	Graphics g2(img);

	paintBeforeEffect(g2);

	PostGraphicsRenderer r(stack, img);
	r.reserveStackOperations(numOps);
	applyPostEffect(r);

	g.drawImageAt(img, 0, 0);
}

void ComponentWithPostGraphicsRenderer::setDrawOverParent(bool shouldDrawOverParent)
{
	drawOverParent = shouldDrawOverParent;
}

void ComponentWithPostGraphicsRenderer::setNumOperations(int numOperations)
{
	numOps = numOperations;
}

}
