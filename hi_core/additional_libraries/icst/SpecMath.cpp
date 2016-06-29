// SpecMath.cpp
// see SpecMathInline.h for source code of fast inline functions
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#if DONT_INCLUDE_HEADERS_IN_CPP
#else
#include "common.h"
#include "SpecMath.h"
#include "MathDefs.h"
#endif

#include <cstring>


						
//******************************************************************************
//* functions
//*
// inverse hyperbolic sine
float SpecMath::asinhf(float x) {return logf(x + sqrtf(x*x + 1.0f));}
double SpecMath::asinh(double x) {return log(x + sqrt(x*x + 1.0));}

// inverse hyperbolic cosine
float SpecMath::acoshf(float x) {return logf(x + sqrtf(x*x - 1.0f));}
double SpecMath::acosh(double x) {return log(x + sqrt(x*x - 1.0));}

// inverse hyperbolic tangent
float SpecMath::atanhf(float x) {return 0.5f*logf((1.0f + x)/(1.0f - x));}
double SpecMath::atanh(double x) {return 0.5*log((1.0 + x)/(1.0 - x));}

// error function, taken from Ooura's math packages
// license as found in the readme file, 19.8.08: *** Copyright(C) 1996 Takuya
// OOURA (email: ooura@mmm.t.u-tokyo.ac.jp). You may use, copy, modify this code
// for any purpose and without fee. You may distribute this ORIGINAL package. ***
// remarks: the code below is unmodified
float SpecMath::erff(float x) {
	return static_cast<float>(erf(static_cast<double>(x)));}
double SpecMath::erf(double x)
{
    int k;
    double w, t, y;
    static double a[65] = {
        5.958930743e-11, -1.13739022964e-9, 
        1.466005199839e-8, -1.635035446196e-7, 
        1.6461004480962e-6, -1.492559551950604e-5, 
        1.2055331122299265e-4, -8.548326981129666e-4, 
        0.00522397762482322257, -0.0268661706450773342, 
        0.11283791670954881569, -0.37612638903183748117, 
        1.12837916709551257377, 
        2.372510631e-11, -4.5493253732e-10, 
        5.90362766598e-9, -6.642090827576e-8, 
        6.7595634268133e-7, -6.21188515924e-6, 
        5.10388300970969e-5, -3.7015410692956173e-4, 
        0.00233307631218880978, -0.0125498847718219221, 
        0.05657061146827041994, -0.2137966477645600658, 
        0.84270079294971486929, 
        9.49905026e-12, -1.8310229805e-10, 
        2.39463074e-9, -2.721444369609e-8, 
        2.8045522331686e-7, -2.61830022482897e-6, 
        2.195455056768781e-5, -1.6358986921372656e-4, 
        0.00107052153564110318, -0.00608284718113590151, 
        0.02986978465246258244, -0.13055593046562267625, 
        0.67493323603965504676, 
        3.82722073e-12, -7.421598602e-11, 
        9.793057408e-10, -1.126008898854e-8, 
        1.1775134830784e-7, -1.1199275838265e-6, 
        9.62023443095201e-6, -7.404402135070773e-5, 
        5.0689993654144881e-4, -0.00307553051439272889, 
        0.01668977892553165586, -0.08548534594781312114, 
        0.56909076642393639985, 
        1.55296588e-12, -3.032205868e-11, 
        4.0424830707e-10, -4.71135111493e-9, 
        5.011915876293e-8, -4.8722516178974e-7, 
        4.30683284629395e-6, -3.445026145385764e-5, 
        2.4879276133931664e-4, -0.00162940941748079288, 
        0.00988786373932350462, -0.05962426839442303805, 
        0.49766113250947636708
    };
    static double b[65] = {
        -2.9734388465e-10, 2.69776334046e-9, 
        -6.40788827665e-9, -1.6678201321e-8, 
        -2.1854388148686e-7, 2.66246030457984e-6, 
        1.612722157047886e-5, -2.5616361025506629e-4, 
        1.5380842432375365e-4, 0.00815533022524927908, 
        -0.01402283663896319337, -0.19746892495383021487, 
        0.71511720328842845913, 
        -1.951073787e-11, -3.2302692214e-10, 
        5.22461866919e-9, 3.42940918551e-9, 
        -3.5772874310272e-7, 1.9999935792654e-7, 
        2.687044575042908e-5, -1.1843240273775776e-4, 
        -8.0991728956032271e-4, 0.00661062970502241174, 
        0.00909530922354827295, -0.2016007277849101314, 
        0.51169696718727644908, 
        3.147682272e-11, -4.8465972408e-10, 
        6.3675740242e-10, 3.377623323271e-8, 
        -1.5451139637086e-7, -2.03340624738438e-6, 
        1.947204525295057e-5, 2.854147231653228e-5, 
        -0.00101565063152200272, 0.00271187003520095655, 
        0.02328095035422810727, -0.16725021123116877197, 
        0.32490054966649436974, 
        2.31936337e-11, -6.303206648e-11, 
        -2.64888267434e-9, 2.050708040581e-8, 
        1.1371857327578e-7, -2.11211337219663e-6, 
        3.68797328322935e-6, 9.823686253424796e-5, 
        -6.5860243990455368e-4, -7.5285814895230877e-4, 
        0.02585434424202960464, -0.11637092784486193258, 
        0.18267336775296612024, 
        -3.67789363e-12, 2.0876046746e-10, 
        -1.93319027226e-9, -4.35953392472e-9, 
        1.8006992266137e-7, -7.8441223763969e-7, 
        -6.75407647949153e-6, 8.428418334440096e-5, 
        -1.7604388937031815e-4, -0.0023972961143507161, 
        0.0206412902387602297, -0.06905562880005864105, 
        0.09084526782065478489
    };

    w = x < 0 ? -x : x;
    if (w < 2.2) {
        t = w * w;
        k = (int) t;
        t -= k;
        k *= 13;
        y = ((((((((((((a[k] * t + a[k + 1]) * t + 
            a[k + 2]) * t + a[k + 3]) * t + a[k + 4]) * t + 
            a[k + 5]) * t + a[k + 6]) * t + a[k + 7]) * t + 
            a[k + 8]) * t + a[k + 9]) * t + a[k + 10]) * t + 
            a[k + 11]) * t + a[k + 12]) * w;
    } else if (w < 6.9) {
        k = (int) w;
        t = w - k;
        k = 13 * (k - 2);
        y = (((((((((((b[k] * t + b[k + 1]) * t + 
            b[k + 2]) * t + b[k + 3]) * t + b[k + 4]) * t + 
            b[k + 5]) * t + b[k + 6]) * t + b[k + 7]) * t + 
            b[k + 8]) * t + b[k + 9]) * t + b[k + 10]) * t + 
            b[k + 11]) * t + b[k + 12];
        y *= y;
        y *= y;
        y *= y;
        y = 1 - y * y;
    } else {
        y = 1;
    }
    return x < 0 ? -y : y;
}

// error function complement, taken from Ooura's math packages
// license as found in the readme file, 19.8.08: *** Copyright(C) 1996 Takuya
// OOURA (email: ooura@mmm.t.u-tokyo.ac.jp). You may use, copy, modify this code
// for any purpose and without fee. You may distribute this ORIGINAL package. ***
// remarks: the code below is unmodified
float SpecMath::erfcf(float x) {
	return static_cast<float>(erfc(static_cast<double>(x)));}
double SpecMath::erfc(double x)
{
	double t, u, y;
	t = 3.97886080735226 / (fabs(x) + 3.97886080735226);
	u = t - 0.5;
	y = (((((((((0.00127109764952614092 * u + 1.19314022838340944e-4) * u - 
		0.003963850973605135) * u - 8.70779635317295828e-4) * u + 
		0.00773672528313526668) * u + 0.00383335126264887303) * u - 
		0.0127223813782122755) * u - 0.0133823644533460069) * u + 
		0.0161315329733252248) * u + 0.0390976845588484035) * u + 
		0.00249367200053503304;
	y = ((((((((((((y * u - 0.0838864557023001992) * u - 
		0.119463959964325415) * u + 0.0166207924969367356) * u + 
		0.357524274449531043) * u + 0.805276408752910567) * u + 
		1.18902982909273333) * u + 1.37040217682338167) * u + 
		1.31314653831023098) * u + 1.07925515155856677) * u + 
		0.774368199119538609) * u + 0.490165080585318424) * u + 
		0.275374741597376782) * t * exp(-x * x);
	return x < 0 ? 2 - y : y;
}

// natural logarithm of the gamma function, x > 0, taken from Ooura's math packages
// license as found in the readme file, 19.8.08: *** Copyright(C) 1996 Takuya
// OOURA (email: ooura@mmm.t.u-tokyo.ac.jp). You may use, copy, modify this code
// for any purpose and without fee. You may distribute this ORIGINAL package. ***
// remarks: the code below is modified to exclude x < 0 for higher speed
double SpecMath::gammaln(double x)
{
    int k;
    double w, t, y, v;
    static double a[22] = {
        9.967270908702825e-5, -1.9831672170162227e-4, 
        -0.00117085315349625822, 0.00722012810948319552, 
        -0.0096221300936780297, -0.04219772092994235254, 
        0.16653861065243609743, -0.04200263501129018037, 
        -0.65587807152061930091, 0.57721566490153514421, 
        0.99999999999999999764, 
        4.67209725901142e-5, -6.812300803992063e-5, 
        -0.00132531159076610073, 0.0073352117810720277, 
        -0.00968095666383935949, -0.0421764281187354028, 
        0.16653313644244428256, -0.04200165481709274859, 
        -0.65587818792782740945, 0.57721567315209190522, 
        0.99999999973565236061
    };
    static double b[98] = {
        -4.587497028e-11, 1.902363396e-10, 
        8.6377323367e-10, 1.15513678861e-8, 
        -2.556403058605e-8, -1.5236723372486e-7, 
        -3.1680510638574e-6, 1.22903704923381e-6, 
        2.334372474572637e-5, 0.00111544038088797696, 
        0.00344717051723468982, 0.03198287045148788384, 
        -0.32705333652955399526, 0.40120442440953927615, 
        -5.184290387e-11, -8.3355121068e-10, 
        -2.56167239813e-9, 1.455875381397e-8, 
        1.3512178394703e-7, 2.9898826810905e-7, 
        -3.58107254612779e-6, -2.445260816156224e-5, 
        -4.417127762011821e-5, 0.00112859455189416567, 
        0.00804694454346728197, 0.04919775747126691372, 
        -0.24818372840948854178, 0.11071780856646862561, 
        3.0279161576e-10, 1.60742167357e-9, 
        -4.05596009522e-9, -5.089259920266e-8, 
        -2.029496209743e-8, 1.35130272477793e-6, 
        3.91430041115376e-6, -2.871505678061895e-5, 
        -2.3052137536922035e-4, 4.5534656385400747e-4, 
        0.01153444585593040046, 0.07924014651650476036, 
        -0.12152192626936502982, -0.07916438300260539592, 
        -5.091914958e-10, -1.15274986907e-9, 
        1.237873512188e-8, 2.937383549209e-8, 
        -3.0621450667958e-7, -7.7409414949954e-7, 
        8.16753874325579e-6, 2.412433382517375e-5, 
        -2.60612176060637e-4, -9.1000087658659231e-4, 
        0.01068093850598380797, 0.11395654404408482305, 
        0.07209569059984075595, -0.10971041451764266684, 
        4.0119897187e-10, -1.3224526679e-10, 
        -1.002723190355e-8, 2.569249716518e-8, 
        2.0336011868466e-7, -1.1809768272606e-6, 
        -3.00660303810663e-6, 4.402212897757763e-5, 
        -1.462405876235375e-5, -0.0016487379559600128, 
        0.00513927520866443706, 0.13843580753590579416, 
        0.32730190978254056722, 0.08588339725978624973, 
        -1.5413428348e-10, 6.4905779353e-10, 
        1.60702811151e-9, -2.655645793815e-8, 
        7.619544277956e-8, 4.7604380765353e-7, 
        -4.90748870866195e-6, 8.21513040821212e-6, 
        1.4804944070262948e-4, -0.00122152255762163238, 
        -8.7425289205498532e-4, 0.1443870369965796831, 
        0.61315889733595543766, 0.55513708159976477557, 
        1.049740243e-11, -2.5832017855e-10, 
        1.39591845075e-9, -2.1177278325e-10, 
        -5.082950464905e-8, 3.7801785193343e-7, 
        -7.3982266659145e-7, -1.088918441519888e-5, 
        1.2491810452478905e-4, -4.9171790705139895e-4, 
        -0.0042570708944826646, 0.13595080378472757216, 
        0.89518356003149514744, 1.31073912535196238583
    };
    static double c[65] = {
        1.16333640008e-8, -8.33156123568e-8, 
        3.832869977018e-7, -1.5814047847688e-6, 
        6.50106723241e-6, -2.74514060128677e-5, 
        1.209015360925566e-4, -5.666333178228163e-4, 
        0.0029294103665559733, -0.0180340086069185819, 
        0.1651788780501166204, 1.1031566406452431944, 
        1.2009736023470742248, 
        1.3842760642e-9, -6.9417501176e-9, 
        3.42976459827e-8, -1.785317236779e-7, 
        9.525947257118e-7, -5.2483007560905e-6, 
        3.02364659535708e-5, -1.858396115473822e-4, 
        0.0012634378559425382, -0.0102594702201954322, 
        0.1243625515195050218, 1.3888709263595291174, 
        2.4537365708424422209, 
        1.298977078e-10, -8.02957489e-10, 
        4.945484615e-9, -3.17563534834e-8, 
        2.092136698089e-7, -1.4252023958462e-6, 
        1.01652510114008e-5, -7.74550502862323e-5, 
        6.537746948291078e-4, -0.006601491253552183, 
        0.0996711934948138193, 1.6110931485817511402, 
        3.9578139676187162939, 
        1.83995642e-11, -1.353537034e-10, 
        9.984676809e-10, -7.6346363974e-9, 
        5.99311464148e-8, -4.868554120177e-7, 
        4.1441957716669e-6, -3.77160856623282e-5, 
        3.805693126824884e-4, -0.0045979851178130194, 
        0.0831422678749791178, 1.7929113303999329439, 
        5.6625620598571415285, 
        3.4858778e-12, -2.97587783e-11, 
        2.557677575e-10, -2.2705728282e-9, 
        2.0702499245e-8, -1.954426390917e-7, 
        1.9343161886722e-6, -2.0479024910257e-5, 
        2.405181940241215e-4, -0.0033842087561074799, 
        0.0713079483483518997, 1.9467574842460867884, 
        7.5343642367587329552
    };
    static double d[7] = {
        -0.00163312359200500807, 8.3644533703385956e-4, 
        -5.9518947575728181e-4, 7.9365057505415415e-4, 
        -0.00277777777735463043, 0.08333333333333309869, 
        0.91893853320467274178
    };

    w = fabs(x);
    if (w < 0.5) {
        k = w < 0.25 ? 0 : 11;
        y = ((((((((((a[k] * w + a[k + 1]) * w + 
            a[k + 2]) * w + a[k + 3]) * w + a[k + 4]) * w + 
            a[k + 5]) * w + a[k + 6]) * w + a[k + 7]) * w + 
            a[k + 8]) * w + a[k + 9]) * w + a[k + 10]) * w;
        y = -log(y);
    } else if (w < 3.5) {
        t = w - 4.5 / (w + 0.5);
        k = ((int) t) + 4;
        t -= k - 3.5;
        k *= 14;
        y = ((((((((((((b[k] * t + b[k + 1]) * t + 
            b[k + 2]) * t + b[k + 3]) * t + b[k + 4]) * t + 
            b[k + 5]) * t + b[k + 6]) * t + b[k + 7]) * t + 
            b[k + 8]) * t + b[k + 9]) * t + b[k + 10]) * t + 
            b[k + 11]) * t + b[k + 12]) * t + b[k + 13];
    } else if (w < 8) {
        k = ((int) w) - 3;
        t = w - (k + 3.5);
        k *= 13;
        y = (((((((((((c[k] * t + c[k + 1]) * t + 
            c[k + 2]) * t + c[k + 3]) * t + c[k + 4]) * t + 
            c[k + 5]) * t + c[k + 6]) * t + c[k + 7]) * t + 
            c[k + 8]) * t + c[k + 9]) * t + c[k + 10]) * t + 
            c[k + 11]) * t + c[k + 12];
    } else {
        v = 1 / w;
        t = v * v;
        y = (((((d[0] * t + d[1]) * t + d[2]) * t + 
            d[3]) * t + d[4]) * t + d[5]) * v + d[6];
        y += (w - 0.5) * log(w) - w;
    }
    return y;
}

// probit function (inverse CDF of the standard normal distribution)
// x = 0..1, free algorithm by Peter J. Acklam, www.math.uio.no/~jacklam
float SpecMath::probit(float x)
{
	static double a[6] = {
		-39.69683028665376, 220.9460984245205,
		-275.9285104469687, 138.3577518672690,
		-30.66479806614716, 2.506628277459239
	};
	static double b[5] = {
		-54.47609879822406, 161.5858368580409,
		-155.6989798598866, 66.80131188771972,
		-13.28068155288572
	};
	static double c[6] = {
		-7.784894002430293e-03, -3.223964580411365e-01,
		-2.400758277161838, -2.549732539343734,
		4.374664141464968, 2.938163982698783
	};
	static double d[4] = {
		7.784695709041462e-03, 3.224671290700398e-01,
		2.445134137142996, 3.754408661907416
	};

	double q, p = static_cast<double>(x);
	if (p <= 0) {return -FLT_MAX;}
	if (p >= 1.0) {return FLT_MAX;}
	if ((p < 0.02425) || (p > 0.97575)) {
		if (p < 0.5) {
			p = sqrt(-2.0*log(p));
			p = (((((c[0]*p + c[1])*p + c[2])*p + c[3])*p + c[4])*p + c[5]) /
					((((d[0]*p + d[1])*p + d[2])*p + d[3])*p + 1.0);
			return static_cast<float>(p);
		}
		else {
			p = sqrt(-2.0*log(1.0 - p));
			p = -(((((c[0]*p + c[1])*p + c[2])*p + c[3])*p + c[4])*p + c[5]) /
					((((d[0]*p + d[1])*p + d[2])*p + d[3])*p + 1.0);
			return static_cast<float>(p);
		}
	}
	else {
		q = p - 0.5;
		p = q*q;
		p = (((((a[0]*p + a[1])*p + a[2])*p + a[3])*p + a[4])*p + a[5])*q /
			(((((b[0]*p + b[1])*p + b[2])*p + b[3])*p + b[4])*p + 1.0);
		return static_cast<float>(p);
	}
}

// fast natural logarithm of the gamma function
// x > 0, rel + abs error < 3e-7
float SpecMath::gammalnf(float x)
{
	static float c[54] = {	
		0.00000000000000f, 0.00000000000000f, 0.00000000000000f,
		0.69314718056074f, 1.79175946922813f, 3.17805383034794f,
		-0.57721109682590f, -0.57721109682590f, 0.42278437230143f,
		0.92278433687666f, 1.25611766862472f, 1.50611766846520f,
		0.82231828535546f, 0.82231828535546f, 0.32246586251322f,
		0.19746697815100f, 0.14191147190726f, 0.11066147683669f,
		-0.39897147122366f, -0.39897147122366f, -0.06733930803989f,
		-0.02568502934247f, -0.01333989057502f, -0.00813161083574f,
		0.26062548157267f, 0.26062548157267f, 0.02050910195132f,
		0.00495252866647f, 0.00186904066032f, 0.00089276626811f,
		-0.17357666698295f, -0.17357666698295f, -0.00715961002874f,
		-0.00112547925798f, -0.00031145097471f, -0.00011701310278f,
		0.09641037192256f, 0.09641037192256f, 0.00245608479886f,
		0.00026773499859f, 0.00005580979152f, 0.00001670756622f,
		-0.03595617151403f, -0.03595617151403f, -0.00066592099606f,
		-0.00005558843847f, -0.00000926795255f, -0.00000229599707f,
		0.00636129048720f, 0.00636129048720f, 0.00009659828319f,
		0.00000680702573f, 0.00000097963960f, 0.00000021323371f	};
			
	x = fabsf(x);
	if (x >= 6.0f) { // Stirling approximation
		float xx = x*x;
		return (x - 0.5f)*logf(x) - x + 0.918938533f
				+ (0.0833333333f*xx - 0.00277777778f)/(xx*x);
	}
	else { // piecewise polynomial approximation + functional equation of gamma
		double y = static_cast<double>(x); 
		int i = fsplit(y);
		float z = static_cast<float>(y);
		z =	c[i] + (c[i+6] + (c[i+12] + (c[i+18] + (c[i+24] + (c[i+30]
			+ (c[i+36] + (c[i+42] + c[i+48]*z)*z)*z)*z)*z)*z)*z)*z;
		if (i == 0) {z -= logf(static_cast<float>(y));}
		return z;
	}
}

// regularized gamma function P(x,y)
// 1e6 > x > 0, y >= 0, return -1 on failure
// (optimization note: faster algorithms exist for large x close to y)
float SpecMath::rgamma(float x, float y)
{
	static const double MINDIV = 1.0e-8*static_cast<double>(FLT_MIN);
	static const double EPSILONS = 2.5e-10;
	static const double EPSILONF = 1.0e-8;
	int i; double a,b,c,d,e,f;
	if (x <= 0) return -1.0f;
	if (y <= 0) return 0;
	double xd = static_cast<double>(x), yd = static_cast<double>(y);
	double lng = gammaln(xd);
	if (yd < (xd + 1.0))	{ // evaluate series
		a = xd; b = c = 1.0/xd;
		for (i=0; i<20000; i++) {
			a += 1.0;
			b *= (yd/a);
			c += b;
			if (b < (c*EPSILONS)) {
				return static_cast<float>(c*exp(xd*log(yd) - yd - lng));
			}
		}
	}
	else { // evaluate continued fraction with Lentz algorithm
		volatile double vtmp;	// compiler won't "optimize" away arithmetic ops 
		b = yd + 1.0 - xd;
		c = 1.0/MINDIV;
		e = d = 1.0/b;
		f = 1.0;
		for (i=0; i<2000; i++) {
			a = f*(xd - f);
			f += 1.0;
			b += 2.0;
			vtmp = b + a*d;
			d = 1.0/(vtmp + MINDIV);
			vtmp = b + a/c;
			c = vtmp + MINDIV;
			a = d*c;
			e *= a;
			if (fabs(a - 1.0) < EPSILONF) {
				return static_cast<float>(1.0 - e*exp(xd*log(yd) - yd - lng));
			}
		}
	}
	return -1.0f;
}

// regularized beta function I(x,a,b)
// x = 0..1, 1e6 > a > 0, 1e6 > b > 0, return -1 on failure
// (optimization note: s. TOMS 708 Didonato/Morris for newer algorithm)
float SpecMath::rbeta(float x, float a, float b)
{
	static const double MINDIV = 1.0e-8*static_cast<double>(FLT_MIN);
	static const double EPSILON = 1.0e-8;
	double tmp,tmp2,tmp3,tmp4, scl = 1.0, ref = 0;
	double ad = static_cast<double>(a);
	double bd = static_cast<double>(b);
	double xd = static_cast<double>(x);
	
	// input preconditioning
	if ((ad <= 0) || (bd <= 0)) return -1.0f;
	if (xd <= 0) return 0;
	if (xd >= 1.0) return 1.0f;
	if ((xd*(ad + bd + 2.0)) >= (ad + 1.0)) { // use reflection property
		xd = 1.0 - xd;
		tmp = ad; ad = bd; bd = tmp;
		scl = -1.0;
		ref = 1.0;
	}
	scl *= exp(	gammaln(ad + bd) - gammaln(ad) - gammaln(bd)
				+ ad*log(xd) + bd*log(1.0 - xd)				);
	
	// evaluate continued fraction with Lentz algorithm
	int i; double c,d,e,n, z1 = ad + bd, z2 = ad + 1.0, z3 = ad - 1.0;
	volatile double vtmp;	// compiler won't "optimize" away arithmetic ops
	c = n = 1.0;
	vtmp = 1.0 - xd*z1/z2;
	e = d = 1.0/(vtmp + MINDIV);
	for (i=0; i<500; i++) {
		tmp = 2.0*n;
		tmp2 = ad + tmp;
		tmp3 = xd*n*(bd - n)/(tmp2*(z3 + tmp));
		tmp4 = xd*(ad + n)*(z1 + n)/(tmp2*(z2 + tmp));
		vtmp = 1.0 + tmp3*d;
		d = 1.0/(vtmp + MINDIV);
		vtmp = 1.0 + tmp3/c;
		c = vtmp + MINDIV;
		e *= (d*c);
		vtmp = 1.0 - tmp4*d;
		d = 1.0/(vtmp + MINDIV);
		vtmp = 1.0 - tmp4/c;
		c = vtmp + MINDIV;
		tmp4 = d*c;
		e *= tmp4;
		if (fabs(tmp4 - 1.0) < EPSILON) {
			return static_cast<float>(ref + scl*e/ad);
		}
		n += 1.0;
	}
	return -1.0f;
}

// bessel function of the first kind Jn(x)
float SpecMath::bessj(float x, int n)
{
	static int mtab[30] = {	12,12,12,14,16,18,20,20,22,24,
							26,28,28,30,30,32,34,36,36,38,
							38,40,40,42,42,44,46,48,48,50	};
	static const double NLIM = 1.0e50;
	static const double INVNLIM = 1.0/NLIM;
	double a,b,c,d,e,tmp,norm, scl = 1.0, xd = static_cast<double>(x);
	int i,m; 
	if (n == 0) {return static_cast<float>(dbesj0(xd));}
	if (n < 0) {n = -n; scl -= 2.0*static_cast<double>(n & 1);}
	if (n == 1) {return static_cast<float>(scl*dbesj1(xd));}
	if (xd > static_cast<double>(n)) { // evaluate directly
		a = dbesj0(xd);
		b = dbesj1(xd);
		c = d = 2.0/xd;
		for (i=1; i<n; i++) {
			tmp = b*c - a;
			a = b;
			b = tmp;
			c += d;
        }
		return static_cast<float>(scl*b);
	}
	else { // evaluate with Miller's algorithm
		if (fabs(xd) < static_cast<double>(FLT_MIN)) {return 0;} 
		if (n < 30) {m = mtab[n];}		// m must be even, theoretically
		else {							// cubic root but sqrt suffices
			m = ((n>>1) + 6 + static_cast<int>(sqrtf(static_cast<float>(n))))<<1;
		}
		a = norm = 0;
		b = 1.0;						// just any nonzero value 
		c = 2.0/xd;
		d = c*static_cast<double>(m);
		e = d - c;
		c *= 2.0;
		for (i=m; i>n; i-=2) {
			a = b*d - a;
			d -= c;
			b = a*e - b;
			e -= c;
			if (fabs(b) > NLIM) {
				a *= INVNLIM;
				b *= INVNLIM;
				norm *= INVNLIM;
			}
			norm += b;
		}
		if (n & 1) {scl *= a; n--;}		// n odd
		else {scl *= b;}				// n even
		for (i=n; i>0; i-=2) {
			a = b*d - a;
			d -= c;
			b = a*e - b;
			e -= c;
			if (fabs(b) > NLIM) {
				a *= INVNLIM;
				b *= INVNLIM;
				norm *= INVNLIM;
				scl *= INVNLIM;
			}
			norm += b;
		}
		return static_cast<float>(scl/(2.0*norm - b));
	}
}

// find root of a function by bisection
// input:	f		=	pointer to the function "func" which has the form
//						"float myfunc(float x, float* p, int n)" with p
//						independent of x
//			p		=	parameters passed to "func"
//			n		=	number of parameters
//			xmax	=	maximum x included in the search
//			xmin	=	minimum x included in the search
//			maxerr	=	maximum absolute error of root,
//						0: highest precision, >0: faster 
// output:	x for which func(x) = 0
// NOTE: xmax and xmin must produce non-zero function values of opposite sign
//		 (otherwise, the function terminates but the root may be incorrect)
float SpecMath::froot(	float (*f)(float,float*,int), float* p, int n,
						float xmax, float xmin, float maxerr			)
{
	float xhi,xlo,xm, eps = __max(maxerr,FLT_MIN);
	if (f(xmin, p, n) < 0) {xhi = xmax; xlo = xmin;}
	else {xhi = xmin; xlo = xmax;}
	xm = 0.5f*(xhi + xlo);
	while ((eps < (xhi - xm)) && ((xm - xlo) > eps)) {
		if (f(xm, p, n) < 0) {xlo = xm;} else {xhi = xm;}
		xm = 0.5f*(xhi + xlo);
	}
	return xm;
}

//******************************************************************************
//* polynomials
//*
// find complex roots r[re(0) im(0)..re(d-1) im(d-1)] of d-th degree polynomial
// sum(i=0..d){c[i]*x^i} with real coefficients c[0..d] using Laguerre's method
// specialties:	very fast, finite runtime -> suitable for realtime processing
//				adaptive precision
//				optimized for signal processing (precision, roots dynamic range)
// return root count (-1: pathologic case, rare in physics-based applications)
// runtime comparison (float version, MSVC6, Matlab 6.1, Pentium M):	
//		polynomial			this routine 			Matlab roots function
//------------------------------------------------------------------------------
//		x^9 + 1					1							20 	 
//
int SpecMath::roots(float* c, float* r, int d)
{
	int i; float* k; k = new float[d+1];
	memcpy(k,c,(d+1)*sizeof(float));
	i = findroot(k,r,d);
	delete[] k;
	return i;
}
int SpecMath::roots(double* c, float* r, int d)
{
	int i;
	double* k; k = new double[d+1];
	double* p; p = new double[2*d];
	for (i=0; i<=d; i++) {k[i] = c[i];}
	i = findroot(k,p,d);
	for (i=0; i<(2*d); i++) {r[i] = static_cast<float>(p[i]);}
	delete[] p; delete[] k;
	return i;
}
int SpecMath::findroot(float* k, float* r, int d)
{
	static const int maxiter = 20;			// max iterations per root
	static const float epsinc = 1.4142135f;	// error bound growth per iteration
	int iter = d*maxiter;					// max overall iterations left
	int i,j,cnt=0,ridx=0; float temp;
	float xre,xim,pre,pim,pdre,pdim,pd2re,pd2im,tre,tim,ure,uim,cxre,eps,n,nm1,b;

	// remove leading/trailing zeroes
	while ((k[d] == 0) && (d > 0)) {d--;}
	while ((k[cnt] == 0) && (d > cnt)) {r[ridx]= r[ridx+1]= 0; ridx+=2; cnt++;}
	n = static_cast<float>(d-cnt); nm1 = n - 1.0f;

fr:	// if degree < 3: find roots and terminate
	if ((d - cnt) < 3) {
		if (cnt == (d-1)) {r[ridx] = -k[d-1]/k[d]; r[ridx+1] = 0;}
		else if (cnt == (d-2)) {
			tre = k[d-1]*k[d-1] - 4.0f*k[d]*k[d-2];
			if (tre >= 0) {
				tre = sqrtf(tre); tim = -0.5f/k[d];
				r[ridx] = tim*(k[d-1] - tre); r[ridx+2] = tim*(k[d-1] + tre);
				r[ridx+1] = 0; r[ridx+3] = 0;
			}
			else {
				tre = sqrtf(-tre); tim = -0.5f/k[d];
				r[ridx] = r[ridx+2] = tim*k[d-1];
				r[ridx+1] = tim*tre; r[ridx+3] = -r[ridx+1];	
			}
		}
		return d;
	}

	// if degree >= 3: find next root
	j=0; xre = xim = tre = 0; eps = FLT_EPSILON;
	b=0; for (i=d-1; i>=cnt; i--) {
		tre = fabsf(k[i]);
		if (tre > b) {b=tre;}
	}
	b = 1.0f + tre/fabsf(k[d]);	
	b *= (4.0f*b);										// (2*Cauchy bound)^2
	while (1) {
it:
		// evaluate polynomial including 1st + 2nd derivative (Horner's method)
		pre = ure = k[d]; pim = pdre = pdim = pd2re = pd2im = uim = 0;
		cxre = xre*(1.0f + eps);
		for (i=d-1; i>=cnt; i--) {
			tre = pd2re; pd2re = pdre + pd2re*xre - pd2im*xim;
			pd2im = pdim + tre*xim + pd2im*xre;
			tre = pdre; pdre = pre + pdre*xre - pdim*xim;
			pdim = pim + tre*xim + pdim*xre;
			tre = pre; pre = k[i] + pre*xre - pim*xim;
			pim = tre*xim + pim*xre;
			tre = ure; ure = k[i] + ure*cxre - uim*xim;
			uim = tre*xim + uim*cxre;
		}
		pd2re *= 2.0f; pd2im *= 2.0f;

		// has the root been approximated to reasonable precision?
		ure -= pre; uim -= pim; tre = ure*ure + uim*uim;
		if ((pre*pre + pim*pim) <= tre) {break;}
		else {
			if (iter <= 0) {return -1;}
			if (j >= maxiter) {	
				xre = 2.0f*static_cast<float>(rand())/RAND_MAX - 1.0f; 
				xim = 0; j=0; eps = FLT_EPSILON;
				goto it;
			}
			else {eps *= epsinc; j++;}
		}

		// calculate new x using Laguerre iteration
		tre = pdre*pdre - pdim*pdim; tim = 2.0f*pdre*pdim;
		ure = pre*pd2re - pim*pd2im; uim = pre*pd2im + pim*pd2re;
		tre = nm1*tre - n*ure; tim = nm1*tim - n*uim;
		tre *= nm1; tim *= nm1;
		temp = sqrtf(tre*tre + tim*tim);
		if (tim >= 0) {tim = sqrtf(0.5f*(temp - tre));}
		else {tim = -sqrtf(0.5f*(temp - tre));}
		tre = sqrtf(0.5f*(temp + tre));
		if ((pdre*tre + pdim*tim) >= 0) {pdre += tre; pdim += tim;}
		else {pdre -= tre; pdim -= tim;}
		ure = pdre*pdre + pdim*pdim;
		if (ure > 0) {
			ure = n/ure;
			tre = pre; pre = pre*pdre + pdim*pim; pim = pdre*pim - pdim*tre;
			pre *= ure; pim *= ure;
			xre -= pre; xim -= pim;
			if ((xre*xre + xim*xim) > b) {
				xre = 2.0f*static_cast<float>(rand())/RAND_MAX - 1.0f; xim = 0;
				j=0; eps = FLT_EPSILON;
			}
		}
		else {
			xre = 2.0f*static_cast<float>(rand())/RAND_MAX - 1.0f; xim = 0;
			j=0; eps = FLT_EPSILON;
		}
		iter--;
	}

	// deflate polynomial
	eps *= sqrtf(eps);							// a measure whether close roots
	ure = xre*xre; uim = xim*xim;				// should form a conjugate pair
	if (uim <= (eps*ure)) {						// real root
		r[ridx] = xre; r[ridx+1] = 0;
		for (i=d; i>0; i--) {k[i-1] += (xre*k[i]);}
		cnt++; ridx += 2; n -= 1.0f; nm1 -= 1.0f;
	}
	else {										// conjugate complex roots
		r[ridx] = r[ridx+2] = xre; r[ridx+1] = xim; r[ridx+3] = -xim;
		tre = -2.0f*xre; tim = ure + uim;
		for (i=d; i>1; i--) {k[i-1] -= (tre*k[i]); k[i-2] -= (tim*k[i]);}
		cnt += 2; ridx += 4; n -= 2.0f; nm1 -= 2.0f;
	}

	goto fr;									// find next root
}
int SpecMath::findroot(double* k, double* r, int d)
{
	static const double MINERR = 5e-7*static_cast<double>(FLT_EPSILON);
	static const int maxiter = 40;				// max iterations per root
	static const double epsinc = 1.4142135;		// error bound growth per iteration
	int iter = d*maxiter;						// max overall iterations left
	int i,j,cnt=0,ridx=0; double temp;
	double xre,xim,pre,pim,pdre,pdim,pd2re,pd2im,tre,tim,ure,uim,cxre,eps,n,nm1,b;

	// remove leading/trailing zeroes
	while ((k[d] == 0) && (d > 0)) {d--;}
	while ((k[cnt] == 0) && (d > cnt)) {r[ridx]= r[ridx+1]= 0; ridx+=2; cnt++;}
	n = static_cast<double>(d-cnt); nm1 = n - 1.0;

fr:	// if degree < 3: find roots and terminate
	if ((d - cnt) < 3) {
		if (cnt == (d-1)) {r[ridx] = -k[d-1]/k[d]; r[ridx+1] = 0;}
		else if (cnt == (d-2)) {
			tre = k[d-1]*k[d-1] - 4.0*k[d]*k[d-2];
			if (tre >= 0) {
				tre = sqrt(tre); tim = -0.5/k[d];
				r[ridx] = tim*(k[d-1] - tre); r[ridx+2] = tim*(k[d-1] + tre);
				r[ridx+1] = 0; r[ridx+3] = 0;
			}
			else {
				tre = sqrt(-tre); tim = -0.5/k[d];
				r[ridx] = r[ridx+2] = tim*k[d-1];
				r[ridx+1] = tim*tre; r[ridx+3] = -r[ridx+1];	
			}
		}
		return d;
	}

	// if degree >= 3: find next root
	j=0; xre = xim = tre = 0; eps = MINERR;
	b=0; for (i=d-1; i>=cnt; i--) {
		tre = fabs(k[i]);
		if (tre > b) {b=tre;}
	}
	b = 1.0 + tre/fabs(k[d]);	
	b *= (4.0*b);										// (2*Cauchy bound)^2
	while (1) {
it:
		// evaluate polynomial including 1st + 2nd derivative (Horner's method)
		pre = ure = k[d]; pim = pdre = pdim = pd2re = pd2im = uim = 0;
		cxre = xre*(1.0 + eps);
		for (i=d-1; i>=cnt; i--) {
			tre = pd2re; pd2re = pdre + pd2re*xre - pd2im*xim;
			pd2im = pdim + tre*xim + pd2im*xre;
			tre = pdre; pdre = pre + pdre*xre - pdim*xim;
			pdim = pim + tre*xim + pdim*xre;
			tre = pre; pre = k[i] + pre*xre - pim*xim;
			pim = tre*xim + pim*xre;
			tre = ure; ure = k[i] + ure*cxre - uim*xim;
			uim = tre*xim + uim*cxre;
		}
		pd2re *= 2.0; pd2im *= 2.0;

		// has the root been approximated to reasonable precision?
		ure -= pre; uim -= pim; tre = ure*ure + uim*uim;
		if ((pre*pre + pim*pim) <= tre) {break;}
		else {
			if (iter <= 0) {return -1;}
			if (j >= maxiter) {	
				xre = 2.0*static_cast<double>(rand())/RAND_MAX - 1.0; 
				xim = 0; j=0; eps = MINERR;
				goto it;
			}
			else {eps *= epsinc; j++;}
		}

		// calculate new x using Laguerre iteration
		tre = pdre*pdre - pdim*pdim; tim = 2.0*pdre*pdim;
		ure = pre*pd2re - pim*pd2im; uim = pre*pd2im + pim*pd2re;
		tre = nm1*tre - n*ure; tim = nm1*tim - n*uim;
		tre *= nm1; tim *= nm1;
		temp = sqrt(tre*tre + tim*tim);
		if (tim >= 0) {tim = sqrt(0.5*(temp - tre));}
		else {tim = -sqrt(0.5*(temp - tre));}
		tre = sqrt(0.5*(temp + tre));
		if ((pdre*tre + pdim*tim) >= 0) {pdre += tre; pdim += tim;}
		else {pdre -= tre; pdim -= tim;}
		ure = pdre*pdre + pdim*pdim;
		if (ure > 0) {
			ure = n/ure;
			tre = pre; pre = pre*pdre + pdim*pim; pim = pdre*pim - pdim*tre;
			pre *= ure; pim *= ure;
			xre -= pre; xim -= pim;
			if ((xre*xre + xim*xim) > b) {
				xre = 2.0*static_cast<double>(rand())/RAND_MAX - 1.0; xim = 0;
				j=0; eps = MINERR;
			}
		}
		else {
			xre = 2.0*static_cast<double>(rand())/RAND_MAX - 1.0; xim = 0;
			j=0; eps = MINERR;
		}
		iter--;
	}

	// deflate polynomial
	eps *= sqrt(eps);							// a measure whether close roots
	ure = xre*xre; uim = xim*xim;				// should form a conjugate pair
	if (uim <= (eps*ure)) {						// real root
		r[ridx] = xre; r[ridx+1] = 0;
		for (i=d; i>0; i--) {k[i-1] += (xre*k[i]);}
		cnt++; ridx += 2; n -= 1.0; nm1 -= 1.0;
	}
	else {										// conjugate complex roots
		r[ridx] = r[ridx+2] = xre; r[ridx+1] = xim; r[ridx+3] = -xim;
		tre = -2.0*xre; tim = ure + uim;
		for (i=d; i>1; i--) {k[i-1] -= (tre*k[i]); k[i-2] -= (tim*k[i]);}
		cnt += 2; ridx += 4; n -= 2.0; nm1 -= 2.0;
	}

	goto fr;									// find next root
}

// construct polynomial from d complex roots r[re(0) im(0)..re(d-1) im(d-1)]
// -> complex coefficients separated into real set cre[0..d] and imaginary
// set cim[0..d], normalization: cre[d]=1,cim[d]=0 
void SpecMath::roottops(double* cre, double* cim, float* r, int d)
{
	int i,j; double xre,xim,temp; 
	cre[0]=1.0; cim[0]=0;
	for (i=0; i<d; i++) {
		xre = static_cast<double>(r[2*i]); xim = static_cast<double>(r[2*i+1]);
		cre[i+1] = cre[i]; cim[i+1] = cim[i];
		for (j=i; j>0; j--) {
			temp = cre[j]; cre[j] = cre[j-1] - xre*cre[j] + xim*cim[j];
			cim[j] = cim[j-1] - xre*cim[j] - xim*temp;
		}
		temp = cre[0]; cre[0] = -xre*cre[0] + xim*cim[0];
		cim[0] = -xre*cim[0] - xim*temp;
	}
}

// symbolic addition of polynomials a[0..da] + b[0..db] -> a[0..max(da,db)]
void SpecMath::addpoly(float* a, int da, float* b, int db)
{
	int i;
	if (da >= db) {for (i=0; i<=db; i++) {a[i] += b[i];}}
	else {
		for (i=0; i<=da; i++) {a[i] += b[i];}
		for (i=da+1; i<=db; i++) {a[i] = b[i];}
	}
}
void SpecMath::addpoly(double* a, int da, double* b, int db)
{
	int i;
	if (da >= db) {for (i=0; i<=db; i++) {a[i] += b[i];}}
	else {
		for (i=0; i<=da; i++) {a[i] += b[i];}
		for (i=da+1; i<=db; i++) {a[i] = b[i];}
	}
}

// symbolic subtraction of polynomials a[0..da] - b[0..db] -> a[0..max(da,db)]
void SpecMath::subpoly(float* a, int da, float* b, int db)
{
	int i;
	if (da >= db) {for (i=0; i<=db; i++) {a[i] -= b[i];}}
	else {
		for (i=0; i<=da; i++) {a[i] -= b[i];}
		for (i=da+1; i<=db; i++) {a[i] = -b[i];}
	}
}
void SpecMath::subpoly(double* a, int da, double* b, int db)
{
	int i;
	if (da >= db) {for (i=0; i<=db; i++) {a[i] -= b[i];}}
	else {
		for (i=0; i<=da; i++) {a[i] -= b[i];}
		for (i=da+1; i<=db; i++) {a[i] = -b[i];}
	}
}

// symbolic multiplication of polynomial with constant a*c[0..d] -> c[0..d]
void SpecMath::mulpoly(float* c, float a, int d) {
	for (int i=0; i<=d; i++) {c[i] *= a;}}
void SpecMath::mulpoly(double* c, double a, int d) {
	for (int i=0; i<=d; i++) {c[i] *= a;}}

// symbolic polynomial multiplication a[0..da] x b[0..db] -> a[0..da+db]
// (note: this operation could also be done with the convolution routine
//	of the BlkDsp class, which would result in a dependency that is
//	avoided here at the price of code duplication)
void SpecMath::pmulpoly(float* a, int da, float* b, int db)
{
	float x;
	int i,j, dtot = da+db;
	for (i=da+1; i<=dtot; i++) {a[i]=0;}
	if (db >= da) {
		for (i=dtot; i>=db; i--) {
			x=0; for (j=i-da; j<=db; j++) {x += (a[i-j]*b[j]);} a[i]=x;}
	}
	else {
		for (i=dtot; i>=da; i--) {
			x=0; for (j=i-da; j<=db; j++) {x += (a[i-j]*b[j]);} a[i]=x;}
		for (i=da-1; i>=db; i--) {
			x=0; for (j=0; j<=db; j++) {x += (a[i-j]*b[j]);} a[i]=x;}
	}
	for (i=db-1; i>=0; i--) {
		x=0; for (j=0; j<=i; j++) {x += (a[i-j]*b[j]);} a[i]=x;} 
}
void SpecMath::pmulpoly(double* a, int da, double* b, int db)
{
	double x;
	int i,j, dtot = da+db;
	for (i=da+1; i<=dtot; i++) {a[i]=0;}
	if (db >= da) {
		for (i=dtot; i>=db; i--) {
			x=0; for (j=i-da; j<=db; j++) {x += (a[i-j]*b[j]);} a[i]=x;}
	}
	else {
		for (i=dtot; i>=da; i--) {
			x=0; for (j=i-da; j<=db; j++) {x += (a[i-j]*b[j]);} a[i]=x;}
		for (i=da-1; i>=db; i--) {
			x=0; for (j=0; j<=db; j++) {x += (a[i-j]*b[j]);} a[i]=x;}
	}
	for (i=db-1; i>=0; i--) {
		x=0; for (j=0; j<=i; j++) {x += (a[i-j]*b[j]);} a[i]=x;} 
}

// symbolic scaling of polynomial c[0..d] -> a^i*c[0..i..d]
void SpecMath::scalepoly(float* c, float a, int d)
{
	float x = a; int i;
	for (i=1; i<=d; i++) {c[i] *= x; x *= a;}
}
void SpecMath::scalepoly(double* c, double a, int d)
{
	double x = a; int i;
	for (i=1; i<=d; i++) {c[i] *= x; x *= a;}
}

// symbolic substitution of polynomials
// replace x of polynomial a[0..da] with polynomial b[0..db] -> a[0..da*db]
void SpecMath::sspoly(float* a, int da, float* b, int db)
{
	int i,j, rsize = da*db + 1;
	float* c = new float[rsize]; 
	c[0] = a[da];
	for (i=da-1,j=0; i>=0; i--,j+=db) {
		pmulpoly(c,j,b,db);
		c[0] += a[i]; 
	}
	memcpy(a,c,rsize*sizeof(float));
	delete[] c;
}
void SpecMath::sspoly(double* a, int da, double* b, int db)
{
	int i,j, rsize = da*db + 1;
	double* c = new double[rsize]; 
	c[0] = a[da];
	for (i=da-1,j=0; i>=0; i--,j+=db) {
		pmulpoly(c,j,b,db);
		c[0] += a[i]; 
	}
	memcpy(a,c,rsize*sizeof(double));
	delete[] c;
}

// reverse polynomial c[0..d]
void SpecMath::revpoly(float* c, int d)
{
	float x; int i, j=d; 
	for (i=0; i<((d+1)>>1); i++,j--) {x=c[i]; c[i]=c[j]; c[j]=x;}
}
void SpecMath::revpoly(double* c, int d)
{
	double x; int i, j=d; 
	for (i=0; i<((d+1)>>1); i++,j--) {x=c[i]; c[i]=c[j]; c[j]=x;}
}

// symbolic differentiation of polynomial c[0..d] -> c[0..d]
void SpecMath::diffpoly(float* c, int d)
{
	float x=1.0f;
	for (int i=0; i<d; i++) {c[i] = x*c[i+1]; x += 1.0f;}
	c[d] = 0;
}
void SpecMath::diffpoly(double* c, int d)
{
	double x=1.0;
	for (int i=0; i<d; i++) {c[i] = x*c[i+1]; x += 1.0;}
	c[d] = 0;
}

// symbolic integration of polynomial c[0..d-1] -> c[0..d]
void SpecMath::integratepoly(float* c, int d)
{
	float x = static_cast<float>(d);
	for (int i=d; i>0; i--) {c[i] = c[i-1]/x; x -= 1.0f;}
	c[0] = 0;
}
void SpecMath::integratepoly(double* c, int d)
{
	double x = static_cast<double>(d);
	for (int i=d; i>0; i--) {c[i] = c[i-1]/x; x -= 1.0;}
	c[0] = 0;
}

// convert d-th degree chebyshev series to power series coefficients c[0..d] 
void SpecMath::chebytops(double* c, int d)
{
	int i,j;
	double* dp; dp = new double[d+1];
	double* dt; dt = new double[d+1];
	for (i=0; i<=d; i++) {dp[i]=0;}
	for (i=0; i<=d; i++) {
		chebypoly(dt,i);
		for (j=0; j<=i; j++) {dp[j] += (c[i]*dt[j]);}
	}
	for (i=0; i<=d; i++) {c[i] = dp[i];}
	delete[] dp; delete[] dt;
}

// convert d-th degree power series to chebyshev series coefficients c[0..d]
void SpecMath::pstocheby(double* c, int d)
{
	int i,j; double a,b,x, y=1.0;
	c[0] *= 2.0;
	for (i=1; i<=d; i++) {
		a = static_cast<double>(i); b = 1.0; 
		x = y*c[i];
		c[i] = 0;
		for (j=i; j>=0; j-=2) {
			c[j] += x;
			x *= (a/b);
			a -= 1.0; b += 1.0;
		}
		y *= 0.5;
	}
	c[0] *= 0.5;
}

// fill c[0..d] with d-th degree chebyshev polynomial of the first kind
void SpecMath::chebypoly(double* c, int d)
{
	int i,j; double x;
	if (d == 0) {c[0]=1.0; return;}
	c[1]=1.0; c[0]=0; 
	double* temp; temp = new double[d+1];
	temp[1]=0; temp[0]=1.0; 
	for (i=2; i<=d; i++) {
		c[i] = 2.0*c[i-1];
		for (j=i-1; j>0; j--) {x=temp[j]; temp[j]=c[j]; c[j] = 2.0*c[j-1]-x;}	
		x=temp[0]; temp[0]=c[0]; c[0] = -x;	
	}
	delete[] temp;
}
											
//******************************************************************************
//* interpolation
//*
// return (re,im) = (extremal x, extremal y) of parabola y(x)
// given y(-1),y(0),y(1), if no extremum return (0,y0)	
Complex SpecMath::paraext(float ym1, float y0, float y1)
{
	float b,c; Complex res;
	b = 0.25f*(y1-ym1); c = 0.5f*(y1+ym1) - y0;
	if (fabsf(b) >= fabsf(c)) {res.re=0; res.im=y0; return res;} 
	res.re = -b/c; res.im = y0 + b*res.re; return res;
}

// fill c with coefficients of y(x) = c[0] + c[1]*x + c[2]*x^2 + c[3]*x^3 such
// that y(x) is an interpolation function for x = x0..x1 that goes through
// (x0,y0) and (x1,y1) with continuous gradients at these points
// 1st version: specify gradients m0 and m1
// 2nd version: specify y[0..3] = ym1,y0,y1,y2 at x[0..3] = xm1,x0,x1,x2
// 3rd version: fast 2nd version with constant interval x[0..3]=(-1,0,1,2)
// 4th version: 3rd version evaluated at a specified x, see SpecMathInline.h
void SpecMath::fitcubic(double* c, float x0, float y0, float m0, 
						float x1, float y1, float m1)
{
	float dx,invdx,m,c1,c2,c3;
	dx = x0 - x1;
	if (fabsf(dx) < FLT_MIN) {
		c[0] = static_cast<double>(0.5f*(y0 + y1)); c[1]=c[2]=c[3]=0; return;}
	invdx = 1.0f/dx;
	m = invdx*(y0 - y1);
	c3 = (m0 + m1 - 2.0f*m)*invdx*invdx;
	c2 = (m0 - m)*invdx - (2.0f*x0 + x1)*c3;
	c1 = m - c2*(x1 + x0) - c3*(x1*x1 + x0*x0 + x1*x0);
	c[0]= static_cast<double>(y0 - ((c3*x0 + c2)*x0 + c1)*x0);
	c[1] = static_cast<double>(c1);
	c[2] = static_cast<double>(c2);
	c[3] = static_cast<double>(c3);
}
void SpecMath::fitcubic(double* c, float* x, float* y)
{
	float dx,m0,m1,mm;
	dx = x[2]-x[1];
	if (fabsf(dx) < FLT_MIN) {
		c[0] = static_cast<double>(0.5f*(y[1]+y[2])); c[1]=c[2]=c[3]=0;
		return;
	}
	else {mm = (y[2]-y[1])/dx;}
	dx = x[1]-x[0];
	if (fabsf(dx) < FLT_MIN) {m0 = mm;}
	else {m0 = 0.5f*(mm + (y[1]-y[0])/dx);}
	dx = x[3]-x[2];
	if (fabsf(dx) < FLT_MIN) {m1 = mm;}
	else {m1 = 0.5f*(mm + (y[3]-y[2])/dx);}
	fitcubic(c,x[1],y[1],m0,x[2],y[2],m1);		
}
void SpecMath::fitcubic(float* c, float* y)
{
	float m0,m1,mm,a;
	m0 = 0.5f*(y[2] - y[0]);
	m1 = 0.5f*(y[3] - y[1]);
	mm = y[2] - y[1];
	a = m0 + m1 - 2.0f*mm;
	c[0]= y[1];
	c[1] = m0;
	c[2] = mm - m0 - a;
	c[3] = a;
}

// fill c with coefficients of chebychev approximation of tabulated y(-1)..y(1),
// table should be large enough to give a good approximation of intermediate
// values using linear interpolation, return max error relative to max(|y|)
// approximation: y(x) = sum(i=0..d){c[i]*T[i](x)}
float SpecMath::chebyapprox(double* c, int d, float* y, int size)
{
	int i,j;
	double x,offset;
	double w = M_PI/static_cast<double>(size);
	double scl = 2.0/static_cast<double>(size);
	double sz = static_cast<double>(size-1);
	double* dp; dp = new double[size];
	
	for (i=0; i<size; i++) {						// evaluate function
		x = cos(w*(static_cast<double>(i)+0.5));
		x = sz*(0.5 + x*0.5);
		offset = floor(x);
		j = static_cast<int>(offset); x -= offset;
		dp[i] = (1.0-x)*static_cast<double>(y[j])+x*static_cast<double>(y[j+1]);
	}
	
	for (j=0; j<=d; j++) {							// calculate coefficients
		x=0;
		for (i=0; i<size; i++) {x += dp[i]*cos(w*j*(i+0.5));}
		c[j]=scl*x;
	}
	c[0] *= 0.5;
	delete[] dp;

	double a1,a2,a3,maxi=0,err=0;					// calculate error
	for (j=0; j<size; j++) {						
		a2 = -2.0 + 4.0*static_cast<double>(j)/sz;
		x=0; a1=0;
		for (i=d; i>=1; i--) {a3=x; x = a2*x - a1 + c[i]; a1=a3;}
		x = 0.5*a2*x - a1 + c[0];
		err = __max(err,fabs(x-static_cast<double>(y[j])));
		maxi = __max(maxi,fabs(static_cast<double>(y[j])));
	}
	return static_cast<float>(err/maxi);
}

//******************************************************************************
//* differential equations
//*
// approximate deltay using the classic 4th order runge-kutta algorithm so that
// y(x + deltax) = y(x) + deltay given the equation dy/dx = func(y(x),p[0..n-1])
// with p independent of x
// input:	y		=	current value of y(x)
//			deltax	=	integration step
//			f		=	pointer to the function "func" which has the form
//						"float myfunc(float y, float* p, int n)"
//			p		=	parameters passed to "func"
//			n		=	number of parameters
// output:	return deltay
// usage:	define function "func" as described above
//			set y to initial value
//			repeat y += rk4(myfunc,p,n,y,deltax) to get consecutive y values
// notes:	handle systems of differential equations as follows:
//			-	call rk4 for each equation passing all system variables y0,y1,..
//				in p excluding the one that is the y of the current call,
//				save the deltay without updating the system variables
//			-	update all system variables at once by adding the deltay 
//			if "func" has to be time-dependent, create a new system variable t
//			with equation dt/dx = 1.0f and pass it to "func" in p	
float SpecMath::rk4(	float (*f)(float,float*,int), float* p, int n,
						float y, float deltax							)
{
	float a,b,c,d, hdeltax = 0.5f*deltax;
	a = f(y, p, n);
	b = f(y + hdeltax*a, p, n);
	c = f(y + hdeltax*b, p, n);
	d = f(y + deltax*c, p, n);
	return deltax*(0.33333333f*(b+c) + 0.16666667f*(a+d));
}
							
//******************************************************************************
//* filter design
//*
// find coefs of bilinear (b[0] + b[1]z^-1)/(1 + a[1]z^-1) that realizes a
// filter with -3dbcorner/shelvingmidpoint frequency fc (relative to fs)
// type: 0=lowpass, 2=highpass, 4=lowshelf, 5=highshelf, 7=allpass
// dbgain = shelving gain in dB
void SpecMath::dzbilin(float* a, float* b, float fc, int type, float dbgain)
{
	float x,y,z, fn = fc*M_PI_FLOAT;
	switch(type) {
	case 0:	x = sinf(fn); y = cosf(fn);						// lowpass
			z = 2.0f*x/(x+y);
			a[1] = z - 1.0f; b[0] = b[1] = 0.5f*z;
			break;
	case 2:	x = sinf(fn); y = cosf(fn);						// highpass
			z = 2.0f*x/(x+y);
			a[1] = z - 1.0f; b[0] = 1.0f - 0.5f*z; b[1] = - b[0];
			break;
	case 4: x = expf(dbgain*logf(10.0f)/40.0f);				// lowshelf
			y = tanf(fn)/x;
			x = x*x*y; z = 1.0f/(1.0f + y);
			a[1] = z*(y - 1.0f); b[0] = z*(x + 1.0f); b[1] = z*(x - 1.0f);
			break;
	case 5: x = expf(dbgain*logf(10.0f)/40.0f);				// highshelf
			y = tanf(fn)/x;
			x = x*x*y; z = 1.0f/(1.0f + y);
			a[1] = z*(1.0f - y); b[0] = z*(x + 1.0f); b[1] = z*(1.0f - x);
			break;
	case 7:	x = sinf(fn); y = cosf(fn);						// allpass
			z = 2.0f*x/(x+y);
			a[1] = b[0] = z - 1.0f; b[1] = 1.0f;
			break;
	default: a[1] = b[1] = 0; b[0] = 1.0f;
	}
	a[0] = 1.0f;
}

// find coefs of biquad (b[0] + b[1]z^-1 + b[2]z^-2)/(1 + a[1]z^-1 + a[2]z^-2)
// of an audio equalizer filter with -3dbcorner/center/shelvingmidpoint frequency
// fc (relative to fs) and quality factor (>0) or -bandwidth in octaves (<=0) qbw,
// type: 0=lowpass, 1=bandpass, 2=highpass, 3=notch, 4=lowshelf, 5=highshelf,
// 6=peaking, 7=allpass, dbgain = peaking/shelving gain in dB
// based on: EQ cookbook formulae by Robert Bristow-Johnson
void SpecMath::eqzbiquad(double* a, double* b, float fc, float qbw,
							int type, float dbgain)
{
	double alpha,tmp,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7,tmp8;
	double omega = 2.0*M_PI*static_cast<double>(fc);
	double sn = sin(omega), cs = cos(omega);
	if (qbw > 0) {qbw = -2.0f/logf(2.0f)*asinhf(0.5f/qbw);}	// Q specified
	alpha = -static_cast<double>(qbw);
	if ((type == 1) || (type == 3) || (type == 6) || (type == 7)) 
	{	// bandwidth prewarping with perception-driven asymmetry compensation 
		alpha = 1.0/(1.0/alpha + 0.03225*omega*omega*omega);	
		alpha *= omega/sn;									
	}
	alpha = sn*sinh(0.5*log(2.0)*alpha);
	switch(type) {
	case 0:													// lowpass
		b[1] = 1.0 - cs; b[0] = b[2] = 0.5*b[1];
		a[0] = 1.0 + alpha; a[1] = -2.0*cs; a[2] = 1.0 - alpha;
		break;
	case 1:													// bandpass
		b[0] = alpha; b[1] = 0; b[2]= -alpha;
		a[0] = 1.0 + alpha; a[1] = -2.0*cs; a[2] = 1.0 - alpha;
		break;
	case 2:													// highpass
		b[1] = -1.0 - cs; b[0] = b[2] = -0.5*b[1];
		a[0] = 1.0 + alpha; a[1] = -2.0*cs; a[2] = 1.0 - alpha;
		break;
	case 3:													// notch
		b[1] = -2.0*cs; b[0] = b[2] = 1.0;
		a[0] = 1.0 + alpha; a[1] = b[1]; a[2] = 1.0 - alpha;
		break;
	case 4:													// lowshelf
		tmp = exp(dbgain*log(10.0)/80.0);
		tmp2 = tmp*tmp;
		tmp3 = tmp2 + 1.0;
		tmp4 = tmp2 - 1.0;
		tmp5 = 2.0*tmp*alpha;
		tmp6 = tmp2*tmp5;
		tmp7 = tmp4*cs;
		tmp8 = tmp3*cs;
		b[0] = b[2] = tmp2*(tmp3 - tmp7); b[0] += tmp6; b[2] -= tmp6;
		b[1] = 2.0*tmp2*(tmp4 - tmp8);
		a[0] = a[2] = tmp3 + tmp7; a[0] += tmp5; a[2] -= tmp5;
		a[1] = -2.0*(tmp4 + tmp8);
		break;
	case 5:													// highshelf
		tmp = exp(dbgain*log(10.0)/80.0);
		tmp2 = tmp*tmp;
		tmp3 = tmp2 + 1.0;
		tmp4 = tmp2 - 1.0;
		tmp5 = 2.0*tmp*alpha;
		tmp6 = tmp2*tmp5;
		tmp7 = tmp4*cs;
		tmp8 = tmp3*cs;
		b[0] = b[2] = tmp2*(tmp3 + tmp7); b[0] += tmp6; b[2] -= tmp6;
		b[1] = -2.0*tmp2*(tmp4 + tmp8);
		a[0] = a[2] = tmp3 - tmp7; a[0] += tmp5; a[2] -= tmp5;
		a[1] = 2.0*(tmp4 - tmp8);
		break;
	case 6:													// peaking
		tmp = exp(dbgain*log(10.0)/40.0);
		tmp2 = alpha*tmp; tmp3 = alpha/tmp;
		b[0] = 1.0 + tmp2; b[1] = -2.0*cs; b[2] = 1.0 - tmp2;
		a[0] = 1.0 + tmp3; a[1] = b[1]; a[2] = 1.0 - tmp3;
		break;
	case 7:													// allpass
		b[0]= 1.0 - alpha; b[1] = -2.0*cs; b[2]= 1.0 + alpha;
		a[0] = b[2]; a[1] = b[1]; a[2] = b[0];
		break;
	default:
		b[0] = 1.0; b[1] = b[2] = 0; a[0] = 1.0; a[1] = a[2] = 0;
	}
	double norm = 1.0/a[0];
	b[0]*=norm; b[1]*=norm; b[2]*=norm; a[1]*=norm; a[2]*=norm; a[0]=1.0;
}

// convert continuous to discrete time transfer function by applying the
// bilinear z-transform, fs = sample rate
// G(s) = (d[order]s^order +...+ d[0])/(c[order]s^order +...+ c[0]) -->
// H(z) = (b[0] +...+ b[order]z^-order)/(1 + a[1]z^-1 +...+ a[order]z^-order)
void SpecMath::stoz(double* a, double* b, float* c, float* d, int order,
					float fs)
{
	int i,j;
	double* np; np = new double[order+1];
	double cx,dx,tmp, twofs = 2.0*static_cast<double>(fs);
	a[0] = static_cast<double>(c[0]); b[0] = static_cast<double>(d[0]);
	np[0] = 1.0;
	for (i=1; i<= order; i++) {
		cx = static_cast<double>(c[i]); dx = static_cast<double>(d[i]);
		np[i] = twofs*np[i-1];
		a[i] = a[i-1] + cx*np[i];
		b[i] = b[i-1] + dx*np[i];									
		for (j=i-1; j>0; j--) {
			np[j] = twofs*(np[j-1] - np[j]);
			a[j] += a[j-1]; a[j] += (cx*np[j]);
			b[j] += b[j-1];	b[j] += (dx*np[j]);
		}	
		np[0] *= -twofs;								
		a[0] += (cx*np[0]);
		b[0] += (dx*np[0]);
	}	
	for (i=0,j=order; i<((order+1)>>1); i++,j--) {
		tmp=a[i]; a[i]=a[j]; a[j]=tmp;
		tmp=b[i]; b[i]=b[j]; b[j]=tmp;
	}
	double norm = 1.0/a[0];
	for (i=0; i<=order; i++) {a[i] *= norm; b[i] *= norm;}
	delete[] np;
}

// find coefficients of bilinear (b[1]s + b[0])/(s + a[0]) that realizes a
// filter with -3dbcorner/shelvingmidpoint frequency fc
// type: 0=lowpass, 2=highpass, 4=lowshelf, 5=highshelf, 7=allpass
// dbgain = shelving gain in dB
void SpecMath::dsbilin(float* a, float* b, float fc, int type, float dbgain)
{
	float g;
	fc *= (2.0f*M_PI_FLOAT);
	switch(type) {
	case 0:	a[0] = fc; b[1] = 0; b[0] = fc; break;			// lowpass
	case 2:	a[0] = fc; b[1] = 1.0f; b[0] = 0; break;		// highpass
	case 4: g = expf(dbgain*logf(10.0f)/40.0f);				// lowshelf
			a[0] = fc/g; b[1] = 1.0f; b[0] = fc*g; break;
	case 5: g = expf(dbgain*logf(10.0f)/40.0f);				// highshelf
			a[0] = fc*g; b[1] = g*g; b[0] = a[0]; break;
	case 7:	a[0] = fc; b[1] = 1.0f; b[0] = -fc; break;		// allpass
	default: a[0] = b[0] = b[1] = 1.0f;
	}
	a[1] = 1.0f;
}

// find coefs of biquad (b[2]s^2 + b[1]s + b[0])/(s^2 + a[1]s + a[0]) that
// realizes a filter with -3dbcorner/center/shelvingmidpoint frequency fc
// and quality factor (>0) or -bandwidth in octaves (<=0) qbw
// type: 0=lowpass, 1=bandpass, 2=highpass, 3=notch, 4=lowshelf, 5=highshelf,
// 6=peaking, 7=allpass, dbgain = peaking/shelving gain in dB
void SpecMath::dsbiquad(float* a, float* b, float fc, float qbw,
								int type, float dbgain)
{
	float g;
	fc *= (2.0f*M_PI_FLOAT);
	if (qbw > 0) {qbw = 1.0f/qbw;}							// use Q
	else {qbw = 2.0f*sinhf(-0.5f*logf(2.0f)*qbw);}			// use bandwidth	
	switch(type) {
	case 0:													// lowpass
		b[2] = b[1] = 0; b[0] = fc*fc;
		a[1] = fc*qbw; a[0] = b[0];
		break;
	case 1:													// bandpass
		b[2] = b[0] = 0; b[1] = fc*qbw;
		a[1] = fc*qbw; a[0] = fc*fc;
		break;
	case 2:													// highpass
		b[2] = 1.0f; b[1] = b[0] = 0;
		a[1] = fc*qbw; a[0] = fc*fc;
		break;
	case 3:													// notch
		b[2] = 1.0f; b[1] = 0; b[0] = fc*fc;
		a[1] = fc*qbw; a[0] = b[0];
		break;
	case 4:													// lowshelf
		g = expf(dbgain*logf(10.0f)/80.0f);
		b[2] = 1.0f; b[1] = g*fc*qbw; b[0] = g*g*fc*fc;
		g = 1.0f/(g*g); a[1] = b[1]*g; a[0] = fc*fc*g;
		break;
	case 5:													// highshelf
		g = expf(dbgain*logf(10.0f)/80.0f);
		b[2] = g*g; b[1] = g*fc*qbw; b[0] = fc*fc;
		a[1] = b[1]; a[0] = b[2]*b[0];
		b[1] *= b[2]; b[0] *= b[2]; b[2] *= b[2];
		break;
	case 6:													// peaking
		g = expf(dbgain*logf(10.0f)/40.0f);
		b[2] = 1.0f; b[1] = g*fc*qbw; b[0] = fc*fc;
		a[1] = fc*qbw/g; a[0] = b[0];
		break;
	case 7:													// allpass
		a[1] = fc*qbw; a[0] = fc*fc;
		b[2] = 1.0f; b[1] = -a[1]; b[0] = a[0];
		break;
	default:
		b[0] = b[1] = b[2] = a[1] = a[0] = 1.0f;
	}
	a[2] = 1.0f;
}

// calculate corner frequencies f[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd)
// and quality factors q[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd) of a
// d-th degree butterworth lowpass filter with corner frequency = 1 realized as
// a series of 2nd-degree lowpass filters, note: the last stage of odd degree
// filters is a 1st-order lowpass which is assigned a quality factor of 0
void SpecMath::fqbutter(float* f, float* q, int d)
{
	float a = M_PI_FLOAT/static_cast<float>(d);
	float b=0.5f;
	if (d & 1) {b += 0.5f; f[(d-1)/2] = 1.0f; q[(d-1)/2] = 0;}	// odd degree
	for (int i=0; i<(d/2); i++) {
		f[i] = 1.0f;
		q[i] = 0.5f/cosf(a*b);
		b += 1.0f;
	}
}

// calculate corner frequencies f[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd)
// and quality factors q[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd) of a
// d-th degree bessel lowpass filter with corner frequency = 1 realized as
// a series of 2nd-degree lowpass filters, note: the last stage of odd degree
// filters is a 1st-order lowpass which is assigned a quality factor of 0
void SpecMath::fqbessel(float* f, float* q, int d)
{
	switch(d) {
	case 1: f[0]=1.0f; q[0]=0;			break;
	case 2:	f[0]=1.2736f; q[0]=0.5773f; break;
	case 3:	f[0]=1.4524f; q[0]=0.6910f;
			f[1]=1.3270f; q[1]=0;		break;
	case 4: f[0]=1.4192f; q[0]=0.5219f;
			f[1]=1.5912f; q[1]=0.8055f; break;
	case 5: f[0]=1.5611f; q[0]=0.5635f;
			f[1]=1.7607f; q[1]=0.9165f;
			f[2]=1.5069f; q[2]=0;		break;
	case 6: f[0]=1.6060f; q[0]=0.5103f;
			f[1]=1.6913f; q[1]=0.6112f;
			f[2]=1.9071f; q[2]=1.0234f; break;
	case 7: f[0]=1.7174f; q[0]=0.5324f;
			f[1]=1.8235f; q[1]=0.6608f;
			f[2]=2.0507f; q[2]=1.1262f;
			f[3]=1.6853f; q[3]=0;		break;
	case 8:	f[0]=1.7837f; q[0]=0.5060f;
			f[1]=1.8376f; q[1]=0.5596f;
			f[2]=1.9591f; q[2]=0.7109f;
			f[3]=2.1953f; q[3]=1.2258f;	break;
	case 9:	f[0]=1.8794f; q[0]=0.5197f;
			f[1]=1.9488f; q[1]=0.5894f;
			f[2]=2.0815f; q[2]=0.7606f;
			f[3]=2.3235f; q[3]=1.3220f;
			f[4]=1.8575f; q[4]=0;		break;
	default:f[0]=1.9490f; q[0]=0.5040f;
			f[1]=1.9870f; q[1]=0.5380f;
			f[2]=2.0680f; q[2]=0.6200f;
			f[3]=2.2110f; q[3]=0.8100f;
			f[4]=2.4850f; q[4]=1.4150f; break;
	}
	for (int i=5; i<=((d-1)/2); i++) {f[i]=FLT_MAX; q[i]=0;}
}

// calculate corner frequencies f[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd)
// and quality factors q[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd) of a
// d-th degree chebyshev type 1 lowpass filter with corner frequency = 1 and
// passband ripple (in db) rdb realized as a series of 2nd-degree lowpass
// filters, note: the last stage of odd degree filters is a 1st-order lowpass
// which is assigned a quality factor of 0
void SpecMath::fqcheby(float* f, float* q, float rdb, int d)
{
	float a = 1.0f/static_cast<float>(d), b=0.5f;
	float g = a*asinhf(1.0f/sqrtf(expf(logf(10.0f)*0.1f*fabsf(rdb)) - 1.0f));
	float c = coshf(g); c *= c;
	float e = sinhf(g);
	float x;
	if (d & 1) {b += 0.5f; f[(d-1)/2] = e; q[(d-1)/2] = 0;}
	a *= M_PI_FLOAT;
	for (int i=0; i<(d/2); i++) {
		x = cosf(a*b);
		f[i] = sqrtf(c - x*x);
		q[i] = 0.5f*f[i]/(e*x);
		b += 1.0f;
	}
}

// convert n normalized lowpass frequencies to those of a lowpass with corner
// frequency fc, if sample rate fs > 0: map frequencies for use with bilinear
// z-transform (e.g. stoz)
void SpecMath::fqlowpass(float* f, float fc, int n, float fs)
{
	if (fs > 0) {fc = fs/M_PI_FLOAT*tanf(M_PI_FLOAT*fc/fs);}
	for (int i=0; i<n; i++) {f[i] *= fc;}
}

// convert n normalized lowpass frequencies to those of a highpass with corner
// frequency fc, if sample rate fs > 0: map frequencies for use with bilinear
// z-transform (e.g. stoz)
void SpecMath::fqhighpass(float* f, float fc, int n, float fs)
{
	if (fs > 0) {fc = fs/M_PI_FLOAT*tanf(M_PI_FLOAT*fc/fs);}
	for (int i=0; i<n; i++) {f[i] = fc/f[i];}
}

// convert series of n normalized lowpass filters to series of 2n 2nd-order
// bandpass filters, if sample rate fs > 0: map frequencies for use with
// bilinear z-transform (e.g. stoz)
// input:	normalized lowpass frequencies f[0..n-1]
//			associated quality factors q[0..n-1], if q<0.5 a single 
//				2nd-order filter is created
//			bandpass center frequency fc and absolute bandwidth bw
// output:	bandpass center frequencies f[0..2*n-1]
//			associated quality factors q[0..2*n-1], 0 indicates "no filter"
//			gain factors g[0..2*n-1] 
void SpecMath::fqbandpass(float* f, float* q, float* g, float fc, float bw,
						  int n, float fs)
{
	float a,b,c,d,e,bws;
	if (fs > 0) {						
		a = M_PI_FLOAT/fs; b = 1.0f/a;
		c = sqrtf(bw*bw + 4.0f*fc*fc);
		d = tanf(0.5f*a*(c + bw)); e = tanf(0.5f*a*(c - bw));
		bw = b*(d - e);
		fc = b*sqrtf(d*e);
	}
	for (int i=n-1; i>=0; i--) {
		if (q[i] < 0.5f) {							// use 1 biquad
			f[2*i] = f[2*i+1] = fc;
			q[2*i] = fc/bw; q[2*i+1] = 0;
			g[2*i] = g[2*i+1] = 1.0f;
		}
		else {										// use 2 biquads
			bw *= f[i];
			bws = bw*bw;
			a = 4.0f*fc*fc;
			b = bws + a;
			c = 0.5f*(b + sqrtf(b*b - a*bws/(q[i]*q[i])));
			d = sqrtf(c), e = sqrtf(c - a);
			f[2*i] = 0.5f*(d - e);
			f[2*i+1] = 0.5f*(d + e);
			q[2*i] = q[2*i+1] = q[i]*d/bw;
			g[2*i] = g[2*i+1] = bw*q[2*i]/fc;
		}
	}
}

//******************************************************************************
//* internal functions
//*
// bessel function J0(x), taken from Ooura's math packages
// license as found in the readme file, 19.8.08: *** Copyright(C) 1996 Takuya
// OOURA (email: ooura@mmm.t.u-tokyo.ac.jp). You may use, copy, modify this code
// for any purpose and without fee. You may distribute this ORIGINAL package. ***
// remarks: the code below is unmodified
double SpecMath::dbesj0(double x)
{
    int k;
    double w, t, y, v, theta;
    static double a[8] = {
        -2.3655394e-12, 4.70889868e-10, 
        -6.78167892231e-8, 6.7816840038636e-6, 
        -4.340277777716935e-4, 0.0156249999999992397, 
        -0.2499999999999999638, 0.9999999999999999997
    };
    static double b[65] = {
        6.26681117e-11, -2.2270614428e-9, 
        6.62981656302e-8, -1.6268486502196e-6, 
        3.21978384111685e-5, -5.00523773331583e-4, 
        0.0059060313537449816, -0.0505265323740109701, 
        0.2936432097610503985, -1.0482565081091638637, 
        1.9181123286040428113, -1.13191994752217001, 
        -0.1965480952704682, 
        4.57457332e-11, -1.5814772025e-9, 
        4.55487446311e-8, -1.0735201286233e-6, 
        2.02015179970014e-5, -2.942392368203808e-4, 
        0.0031801987726150648, -0.0239875209742846362, 
        0.1141447698973777641, -0.2766726722823530233, 
        0.1088620480970941648, 0.5136514645381999197, 
        -0.2100594022073706033, 
        3.31366618e-11, -1.1119090229e-9, 
        3.08823040363e-8, -6.956602653104e-7, 
        1.23499947481762e-5, -1.66295194539618e-4, 
        0.0016048663165678412, -0.0100785479932760966, 
        0.0328996815223415274, -0.0056168761733860688, 
        -0.2341096400274429386, 0.2551729256776404262, 
        0.2288438186148935667, 
        2.38007203e-11, -7.731046439e-10, 
        2.06237001152e-8, -4.412291442285e-7, 
        7.3107766249655e-6, -8.91749801028666e-5, 
        7.34165451384135e-4, -0.0033303085445352071, 
        0.0015425853045205717, 0.0521100583113136379, 
        -0.1334447768979217815, -0.1401330292364750968, 
        0.2685616168804818919, 
        1.6935595e-11, -5.308092192e-10, 
        1.35323005576e-8, -2.726650587978e-7, 
        4.151324014176e-6, -4.43353052220157e-5, 
        2.815740758993879e-4, -4.393235121629007e-4, 
        -0.0067573531105799347, 0.0369141914660130814, 
        0.0081673361942996237, -0.257338128589888186, 
        0.0459580257102978932
    };
    static double c[70] = {
        -3.009451757e-11, -1.4958003844e-10, 
        5.06854544776e-9, 1.863564222012e-8, 
        -6.0304249068078e-7, -1.47686259937403e-6, 
        4.714331342682714e-5, 6.286305481740818e-5, 
        -0.00214137170594124344, -8.9157336676889788e-4, 
        0.04508258728666024989, -0.00490362805828762224, 
        -0.27312196367405374426, 0.04193925184293450356, 
        -7.1245356e-12, -4.1170814825e-10, 
        1.38012624364e-9, 5.704447670683e-8, 
        -1.9026363528842e-7, -5.33925032409729e-6, 
        1.736064885538091e-5, 3.0692619152608375e-4, 
        -9.2598938200644367e-4, -0.00917934265960017663, 
        0.02287952522866389076, 0.10545197546252853195, 
        -0.16126443075752985095, -0.19392874768742235538, 
        2.128344556e-11, -3.1053910272e-10, 
        -3.34979293158e-9, 4.50723289505e-8, 
        3.6437959146427e-7, -4.46421436266678e-6, 
        -2.523429344576552e-5, 2.7519882931758163e-4, 
        9.7185076358599358e-4, -0.00898326746345390692, 
        -0.01665959196063987584, 0.11456933464891967814, 
        0.07885001422733148815, -0.23664819446234712621, 
        3.035295055e-11, 5.486066835e-11, 
        -5.01026824811e-9, -5.0124684786e-9, 
        5.8012340163034e-7, 1.6788922416169e-7, 
        -4.373270270147275e-5, 1.183898532719802e-5, 
        0.00189863342862291449, -0.0011375924956163613, 
        -0.03846797195329871681, 0.02389746880951420335, 
        0.22837862066532347461, -0.06765394811166522844, 
        1.279875977e-11, 3.5925958103e-10, 
        -2.28037105967e-9, -4.852770517176e-8, 
        2.8696428000189e-7, 4.40131125178642e-6, 
        -2.366617753349105e-5, -2.4412456252884129e-4, 
        0.00113028178539430542, 0.0070847051391978908, 
        -0.02526914792327618386, -0.08006137953480093426, 
        0.16548380461475971846, 0.14688405470042110229
    };
    static double d[52] = {
        1.059601355592185731e-14, -2.71150591218550377e-13, 
        8.6514809056201638e-12, -4.6264028554286627e-10, 
        5.0815403835647104e-8, -1.76722552048141208e-5, 
        0.16286750396763997378, 2.949651820598278873e-13, 
        -8.818215611676125741e-12, 3.571119876162253451e-10, 
        -2.63192412099371706e-8, 4.709502795656698909e-6, 
        -0.005208333333333283282, 
        7.18344107717531977e-15, -2.51623725588410308e-13, 
        8.6017784918920604e-12, -4.6256876614290359e-10, 
        5.0815343220437937e-8, -1.7672255176494197e-5, 
        0.16286750396763433767, 2.2327570859680094777e-13, 
        -8.464594853517051292e-12, 3.563766464349055183e-10, 
        -2.631843986737892965e-8, 4.70950234228865941e-6, 
        -0.0052083333332278466225, 
        5.15413392842889366e-15, -2.27740238380640162e-13, 
        8.4827767197609014e-12, -4.6224753682737618e-10, 
        5.0814848128929134e-8, -1.7672254763876748e-5, 
        0.16286750396748926663, 1.7316195320192170887e-13, 
        -7.971122772293919646e-12, 3.544039469911895749e-10, 
        -2.631443902081701081e-8, 4.709498228695400603e-6, 
        -0.005208333331514365361, 
        3.84653681453798517e-15, -2.04464520778789011e-13, 
        8.3089298605177838e-12, -4.6155016158412096e-10, 
        5.081326369646665e-8, -1.76722528311426167e-5, 
        0.1628675039665006593, 1.3797879972460878797e-13, 
        -7.448089381011684812e-12, 3.51273379710695978e-10, 
        -2.630500895563592722e-8, 4.709483934775839193e-6, 
        -0.0052083333227940760113
    };

    w = fabs(x);
    if (w < 1) {
        t = w * w;
        y = ((((((a[0] * t + a[1]) * t + 
            a[2]) * t + a[3]) * t + a[4]) * t + 
            a[5]) * t + a[6]) * t + a[7];
    } else if (w < 8.5) {
        t = w * w * 0.0625;
        k = (int) t;
        t -= k + 0.5;
        k *= 13;
        y = (((((((((((b[k] * t + b[k + 1]) * t + 
            b[k + 2]) * t + b[k + 3]) * t + b[k + 4]) * t + 
            b[k + 5]) * t + b[k + 6]) * t + b[k + 7]) * t + 
            b[k + 8]) * t + b[k + 9]) * t + b[k + 10]) * t + 
            b[k + 11]) * t + b[k + 12];
    } else if (w < 12.5) {
        k = (int) w;
        t = w - (k + 0.5);
        k = 14 * (k - 8);
        y = ((((((((((((c[k] * t + c[k + 1]) * t + 
            c[k + 2]) * t + c[k + 3]) * t + c[k + 4]) * t + 
            c[k + 5]) * t + c[k + 6]) * t + c[k + 7]) * t + 
            c[k + 8]) * t + c[k + 9]) * t + c[k + 10]) * t + 
            c[k + 11]) * t + c[k + 12]) * t + c[k + 13];
    } else {
        v = 24 / w;
        t = v * v;
        k = 13 * ((int) t);
        y = ((((((d[k] * t + d[k + 1]) * t + 
            d[k + 2]) * t + d[k + 3]) * t + d[k + 4]) * t + 
            d[k + 5]) * t + d[k + 6]) * sqrt(v);
        theta = (((((d[k + 7] * t + d[k + 8]) * t + 
            d[k + 9]) * t + d[k + 10]) * t + d[k + 11]) * t + 
            d[k + 12]) * v - 0.78539816339744830962;
        y *= cos(w + theta);
    }
    return y;
}

// bessel function J1(x), taken from Ooura's math packages
// license as found in the readme file, 19.8.08: *** Copyright(C) 1996 Takuya
// OOURA (email: ooura@mmm.t.u-tokyo.ac.jp). You may use, copy, modify this code
// for any purpose and without fee. You may distribute this ORIGINAL package. ***
// remarks: the code below is unmodified
double SpecMath::dbesj1(double x)
{
    int k;
    double w, t, y, v, theta;
    static double a[8] = {
        -1.4810349e-13, 3.363594618e-11, 
        -5.65140051697e-9, 6.7816840144764e-7, 
        -5.425347222188379e-5, 0.00260416666666662438, 
        -0.06249999999999999799, 0.49999999999999999998
    };
    static double b[65] = {
        2.43721316e-12, -9.400554763e-11, 
        3.0605338998e-9, -8.287270492518e-8, 
        1.83020515991344e-6, -3.219783841164382e-5, 
        4.3795830161515318e-4, -0.00442952351530868999, 
        0.03157908273375945955, -0.14682160488052520107, 
        0.39309619054093640008, -0.4795280821510107028, 
        0.1414899934402712514, 
        1.82119257e-12, -6.862117678e-11, 
        2.1732790836e-9, -5.69359291782e-8, 
        1.20771046483277e-6, -2.020151799736374e-5, 
        2.5745933218048448e-4, -0.00238514907946126334, 
        0.01499220060892984289, -0.05707238494868888345, 
        0.10375225210588234727, -0.02721551202427354117, 
        -0.06420643306727498985, 
        1.352611196e-12, -4.9706947875e-11, 
        1.527944986332e-9, -3.8602878823401e-8, 
        7.82618036237845e-7, -1.23499947484511e-5, 
        1.45508295194426686e-4, -0.001203649737425854162, 
        0.006299092495799005109, -0.016449840761170764763, 
        0.002106328565019748701, 0.05852741000686073465, 
        -0.031896615709705053191, 
        9.97982124e-13, -3.5702556073e-11, 
        1.062332772617e-9, -2.5779624221725e-8, 
        4.96382962683556e-7, -7.310776625173004e-6, 
        7.8028107569541842e-5, -5.50624088538081113e-4, 
        0.002081442840335570371, -7.71292652260286633e-4, 
        -0.019541271866742634199, 0.033361194224480445382, 
        0.017516628654559387164, 
        7.31050661e-13, -2.5404499912e-11, 
        7.29360079088e-10, -1.6915375004937e-8, 
        3.06748319652546e-7, -4.151324014331739e-6, 
        3.8793392054271497e-5, -2.11180556924525773e-4, 
        2.74577195102593786e-4, 0.003378676555289966782, 
        -0.013842821799754920148, -0.002041834048574905921, 
        0.032167266073736023299
    };
    static double c[70] = {
        -1.185964494e-11, 3.9110295657e-10, 
        1.80385519493e-9, -5.575391345723e-8, 
        -1.8635897017174e-7, 5.42738239401869e-6, 
        1.181490114244279e-5, -3.300031939852107e-4, 
        -3.7717832892725053e-4, 0.01070685852970608288, 
        0.00356629346707622489, -0.13524776185998074716, 
        0.00980725611657523952, 0.27312196367405374425, 
        -3.029591097e-11, 9.259293559e-11, 
        4.96321971223e-9, -1.518137078639e-8, 
        -5.7045127595547e-7, 1.71237271302072e-6, 
        4.271400348035384e-5, -1.2152454198713258e-4, 
        -0.00184155714921474963, 0.00462994691003219055, 
        0.03671737063840232452, -0.06863857568599167175, 
        -0.21090395092505707655, 0.16126443075752985095, 
        -2.19760208e-11, -2.7659100729e-10, 
        3.74295124827e-9, 3.684765777023e-8, 
        -4.5072801091574e-7, -3.27941630669276e-6, 
        3.5713715545163e-5, 1.7664005411843533e-4, 
        -0.00165119297594774104, -0.00485925381792986774, 
        0.03593306985381680131, 0.04997877588191962563, 
        -0.22913866929783936544, -0.07885001422733148814, 
        5.16292316e-12, -3.9445956763e-10, 
        -6.6220021263e-10, 5.511286218639e-8, 
        5.01257940078e-8, -5.22111059203425e-6, 
        -1.34311394455105e-6, 3.0612891890766805e-4, 
        -7.103391195326182e-5, -0.00949316714311443491, 
        0.00455036998246516948, 0.11540391585989614784, 
        -0.04779493761902840455, -0.2283786206653234746, 
        2.697817493e-11, -1.6633326949e-10, 
        -4.3313486035e-9, 2.508404686362e-8, 
        4.8528284780984e-7, -2.58267851112118e-6, 
        -3.521049080466759e-5, 1.6566324273339952e-4, 
        0.00146474737522491617, -0.00565140892697147306, 
        -0.028338820556793004, 0.07580744376982855057, 
        0.16012275906960187978, -0.16548380461475971845
    };
    static double d[52] = {
        -1.272346002224188092e-14, 3.370464692346669075e-13, 
        -1.144940314335484869e-11, 6.863141561083429745e-10, 
        -9.491933932960924159e-8, 5.301676561445687562e-5, 
        0.162867503967639974, -3.652982212914147794e-13, 
        1.151126750560028914e-11, -5.165585095674343486e-10, 
        4.657991250060549892e-8, -1.186794704692706504e-5, 
        0.01562499999999994026, 
        -8.713069680903981555e-15, 3.140780373478474935e-13, 
        -1.139089186076256597e-11, 6.862299023338785566e-10, 
        -9.491926788274594674e-8, 5.301676558106268323e-5, 
        0.162867503967646622, -2.792555727162752006e-13, 
        1.108650207651756807e-11, -5.156745588549830981e-10, 
        4.657894859077370979e-8, -1.186794650130550256e-5, 
        0.01562499999987299901, 
        -6.304859171204770696e-15, 2.857249044208791652e-13, 
        -1.124956921556753188e-11, 6.858482894906716661e-10, 
        -9.49186795351689846e-8, 5.301676509057781574e-5, 
        0.1628675039678191167, -2.185193490132496053e-13, 
        1.048820673697426074e-11, -5.132819367467680132e-10, 
        4.65740943737299422e-8, -1.186794150862988921e-5, 
        0.01562499999779270706, 
        -4.74041720979200985e-15, 2.578715253644144182e-13, 
        -1.104148898414138857e-11, 6.850134201626289183e-10, 
        -9.49167823417491964e-8, 5.301676277588728159e-5, 
        0.1628675039690033136, -1.75512205749384229e-13, 
        9.848723331445182397e-12, -5.094535425482245697e-10, 
        4.656255982268609304e-8, -1.186792402114394891e-5, 
        0.01562499998712198636
    };

    w = fabs(x);
    if (w < 1) {
        t = w * w;
        y = (((((((a[0] * t + a[1]) * t + 
            a[2]) * t + a[3]) * t + a[4]) * t + 
            a[5]) * t + a[6]) * t + a[7]) * w;
    } else if (w < 8.5) {
        t = w * w * 0.0625;
        k = (int) t;
        t -= k + 0.5;
        k *= 13;
        y = ((((((((((((b[k] * t + b[k + 1]) * t + 
            b[k + 2]) * t + b[k + 3]) * t + b[k + 4]) * t + 
            b[k + 5]) * t + b[k + 6]) * t + b[k + 7]) * t + 
            b[k + 8]) * t + b[k + 9]) * t + b[k + 10]) * t + 
            b[k + 11]) * t + b[k + 12]) * w;
    } else if (w < 12.5) {
        k = (int) w;
        t = w - (k + 0.5);
        k = 14 * (k - 8);
        y = ((((((((((((c[k] * t + c[k + 1]) * t + 
            c[k + 2]) * t + c[k + 3]) * t + c[k + 4]) * t + 
            c[k + 5]) * t + c[k + 6]) * t + c[k + 7]) * t + 
            c[k + 8]) * t + c[k + 9]) * t + c[k + 10]) * t + 
            c[k + 11]) * t + c[k + 12]) * t + c[k + 13];
    } else {
        v = 24 / w;
        t = v * v;
        k = 13 * ((int) t);
        y = ((((((d[k] * t + d[k + 1]) * t + 
            d[k + 2]) * t + d[k + 3]) * t + d[k + 4]) * t + 
            d[k + 5]) * t + d[k + 6]) * sqrt(v);
        theta = (((((d[k + 7] * t + d[k + 8]) * t + 
            d[k + 9]) * t + d[k + 10]) * t + d[k + 11]) * t + 
            d[k + 12]) * v - 0.78539816339744830962;
        y *= sin(w + theta);
    }
    return x < 0 ? -y : y;
}


