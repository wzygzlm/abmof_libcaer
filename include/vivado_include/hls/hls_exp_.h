/*****************************************************************************
 *
 *     Author: Xilinx, Inc.
 *
 *     This text contains proprietary, confidential information of
 *     Xilinx, Inc. , is distributed by under license from Xilinx,
 *     Inc., and may be used, copied and/or disclosed only pursuant to
 *     the terms of a valid license agreement with Xilinx, Inc.
 *
 *     XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS"
 *     AS A COURTESY TO YOU, SOLELY FOR USE IN DEVELOPING PROGRAMS AND
 *     SOLUTIONS FOR XILINX DEVICES.  BY PROVIDING THIS DESIGN, CODE,
 *     OR INFORMATION AS ONE POSSIBLE IMPLEMENTATION OF THIS FEATURE,
 *     APPLICATION OR STANDARD, XILINX IS MAKING NO REPRESENTATION
 *     THAT THIS IMPLEMENTATION IS FREE FROM ANY CLAIMS OF INFRINGEMENT,
 *     AND YOU ARE RESPONSIBLE FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE
 *     FOR YOUR IMPLEMENTATION.  XILINX EXPRESSLY DISCLAIMS ANY
 *     WARRANTY WHATSOEVER WITH RESPECT TO THE ADEQUACY OF THE
 *     IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OR
 *     REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM CLAIMS OF
 *     INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE.
 *
 *     Xilinx products are not intended for use in life support appliances,
 *     devices, or systems. Use in such applications is expressly prohibited.
 *
 *     (c) Copyright 2012-2016 Xilinx Inc.
 *     All rights reserved.
 *
 *****************************************************************************/

/**
 * @file hls_exp.h
 *
 * see: J.Detrey and F.d.Dinechin, "A parameterized floating-point exponential function for FPGA"
 */
namespace exp_reduce_ {

#include "hls_exp_tables_.h"

template<typename T>
class exp_traits {};

template <> class exp_traits<half>
{
public:
    const static int we = fp_struct<half>::EXP_BITS;
    const static int wf = fp_struct<half>::SIG_BITS; // 10

    const static int gbits = 3;
    const static int gbits_Z2 = 3;
    const static int w_Z1 = 9;

    // input ap_ufixed < 4, -9 >
    // output ap_uifxed < 5, -8 >
    static ap_ufixed<wf+gbits_Z2-w_Z1+1, -w_Z1+1> exp_Z1P_m_1 ( ap_ufixed<wf+gbits-w_Z1,-w_Z1> Z1P ) {
        ap_ufixed<wf+gbits_Z2-w_Z1+1, -w_Z1+1> Z1P_l = Z1P;
        return Z1P_l;
    }
};

template <> class exp_traits<float>
{
public:
    const static int we = fp_struct<float>::EXP_BITS;
    const static int wf = fp_struct<float>::SIG_BITS; // 23

    const static int gbits = 4;
    const static int gbits_Z2 = 3;
    const static int w_Z1 = 9;

    // input ap_ufixed < 18, -9 >
    // output ap_ufixed < 18, -8 >
    // Z = Z1 + Z1P = Z1 + Z2
    // e^Z1P - 1 = Z1P + Z2^2/2 = Z1P + f(Z2)
    // w_Z2_ind = w - w/2 - w1 = 5
    static ap_ufixed<wf+gbits_Z2-w_Z1+1, -w_Z1+1> exp_Z1P_m_1 ( ap_ufixed<wf+gbits-w_Z1,-w_Z1> Z1P ) {
        const static int w_Z2_ind = 5;
        ap_uint<w_Z2_ind> Z2_ind = Z1P ( Z1P.wl()-1 , Z1P.wl()-w_Z2_ind );
        ap_ufixed<wf+gbits-2*w_Z1-1,-2*w_Z1-1> f_Z2 = table_f_Z2< ap_ufixed<wf+gbits,0> >::array [ Z2_ind ]; // < 8, -19 >
        ap_ufixed<wf+gbits_Z2-w_Z1+1, -w_Z1+1> exp_Z1P_m_1 = Z1P + f_Z2;
        return exp_Z1P_m_1;
    }
};

template <> class exp_traits<double>
{
public:
    const static int we = fp_struct<double>::EXP_BITS;
    const static int wf = fp_struct<double>::SIG_BITS; // 52

    // guard bits should be 7bit
    // but to reduce size of multiplier
    // use 7bit for exp_Z3_m_1 * exp_Z4_m_1 and exp_Z2_m_1 * exp_Z2P_m_1
    // use 5bit for exp_Z1 * exp_Z1P_m_1
    const static int gbits = 7;
    const static int gbits_Z2 = 5;
    const static int w_Z1 = 8;

    // input ap_ufixed < 51, -8 >
    // output ap_ufixed < 50, -7 >
    // Z = Z1 + Z2 + Z3 + Z4, w = 59, w1 = 8, w2 = 8, w3 = 8, w4 = 35
    static ap_ufixed<wf+gbits_Z2-w_Z1+1, -w_Z1+1> exp_Z1P_m_1 ( ap_ufixed<wf+gbits-w_Z1,-w_Z1> Z1P ) {
        const static int w_Z2 = 8;
	const static int w_Z2P = 43;
        const static int w_Z3 = 8;
        const static int w_Z4 = 35;

	ap_ufixed < w_Z2  , -w_Z1           >	Z2  = Z1P; // Z1P [ 50 .. 43 ]
	ap_ufixed < w_Z2P , -w_Z1-w_Z2      >	Z2P = Z1P; // Z1P [ 42 ..  0 ]
	ap_ufixed < w_Z3  , -w_Z1-w_Z2      >	Z3  = Z1P; // Z1P [ 42 .. 35 ]
	ap_ufixed < w_Z4  , -w_Z1-w_Z2-w_Z3 >	Z4  = Z1P; // Z1P [ 34 ..  0 ]

	// e^Z = e^Z1 * e^Z2 * e^Z3 * e^Z4
	//     = e^Z1 * ( 1 + Z2 + f(Z2) ) * ( 1 + Z3 + f(Z3) ) * ( 1 + Z4 + f(Z4) )
	//
	// 1. let's start from
	//   e^Z2P - 1
	// = ( 1 + Z3 + f(Z3) ) * ( 1 + Z4 + f(Z4) ) - 1
	// = ( Z3 + f(Z3) ) + ( Z4 + f(Z4) ) + ( Z3 + f(Z3) ) * ( Z4 + f(Z4) )
	//
	// 1.1 Z4 + f(Z4)
	// w_Z4_ind = w4 - w/2 = 6
	// instead of using a f_Z4 table, we reuse f_Z3 table for f_Z4
        const static int w_Z4_ind = w_Z3;
	ap_uint<w_Z4_ind> Z4_ind = Z4 ( Z4.wl()-1 , Z4.wl()-w_Z4_ind );
	ap_ufixed<wf+gbits-2*(w_Z1+w_Z2+w_Z3)-1,-2*(w_Z1+w_Z2+w_Z3)-1> f_Z4 = table_f_Z3< ap_ufixed<wf+gbits,0> >::array [ Z4_ind ] >> ( 2*w_Z3 ); // < 10, -49 >
	ap_ufixed<wf+gbits-w_Z1-w_Z2-w_Z3+1,-w_Z1-w_Z2-w_Z3+1> exp_Z4_m_1 = Z4 + f_Z4; // < 36, -23 >

	// 1.2 Z3 + f(Z3)
	ap_uint<w_Z3> Z3_ind = Z3 ( Z3.wl()-1, 0 );
        ap_ufixed<wf+gbits-2*(w_Z1+w_Z2)-1,-2*(w_Z1+w_Z2)-1> f_Z3 = table_f_Z3< ap_ufixed<wf+gbits,0> >::array [ Z3_ind ]; // < 26, -33 >
        ap_ufixed<wf+gbits-w_Z1-w_Z2+1,-w_Z1-w_Z2+1> exp_Z3_m_1 = Z3 + f_Z3; // < 44, -15 >

	// 1.3 ( Z3 + f(Z3) ) * ( Z4 + f(Z4) )
	ap_ufixed<wf+gbits-2*(w_Z1+w_Z2)-w_Z3+2,-2*(w_Z1+w_Z2)-w_Z3+2> exp_Z2P_m_1_lo = exp_Z3_m_1 * exp_Z4_m_1; // < 21, -38 >

        // 1.4 e^Z2P - 1
	ap_ufixed<wf+gbits-w_Z1-w_Z2+2,-w_Z1-w_Z2+2> exp_Z2P_m_1_l = exp_Z3_m_1 + exp_Z4_m_1 + exp_Z2P_m_1_lo; // < 45, -14 >
	assert( exp_Z2P_m_1_l[exp_Z2P_m_1_l.wl()-1] == 0 );
	ap_ufixed<wf+gbits-w_Z1-w_Z2+1,-w_Z1-w_Z2+1> exp_Z2P_m_1 = exp_Z2P_m_1_l; // < 44, -15 >

	// 2. e^Z1P - 1
	//  = ( 1 + Z2 + f(Z2) ) * ( 1 + ( e^Z2P - 1 ) ) - 1
	//  = ( Z2 + f(Z2) ) + ( e^Z2P - 1 ) + ( Z2 + f(Z2) ) * ( e^Z2P - 1 )
	//
	// 2.1 Z2 + f(Z2)
        ap_uint<w_Z2> Z2_ind = Z2 ( Z2.wl()-1, 0 );
        ap_ufixed<wf+gbits-2*w_Z1-1,-2*w_Z1-1> f_Z2 = table_f_Z2< ap_ufixed<wf+gbits,0> >::array [ Z2_ind ]; // < 42, -17 >
	ap_ufixed<wf+gbits-w_Z1+1 -2,-w_Z1+1> exp_Z2_m_1 = Z2 + f_Z2; // < 52, -7 > -> < 50, -7> to save multipliers

	// 2.2 ( Z2 + f(Z2) ) * ( e^Z2P - 1 )
	ap_ufixed<wf+gbits-2*w_Z1-w_Z2+2,-2*w_Z1-w_Z2+2> exp_Z1P_m_1_lo = exp_Z2_m_1 * exp_Z2P_m_1; // < 37, -22 >

	// 2.3 e^Z1P - 1
        ap_ufixed<wf+gbits-w_Z1+2,-w_Z1+2> exp_Z1P_m_1_l = exp_Z2_m_1 + exp_Z2P_m_1 + exp_Z1P_m_1_lo; // < 53, -6 >
        assert( exp_Z1P_m_1_l[exp_Z1P_m_1_l.wl()-1] == 0 );
        ap_ufixed<wf+gbits_Z2-w_Z1+1,-w_Z1+1> exp_Z1P_m_1 = exp_Z1P_m_1_l; // < 50, -7 >

	return exp_Z1P_m_1;
    }
};

template<typename T>
T exp_generic(T x)
{
#pragma HLS pipeline

	fp_struct<T> es(x);
        const static int we = exp_traits<T>::we;
        const static int wf = exp_traits<T>::wf;

        fp_struct<T> out;
	out.sign[0] = 0;
	out.sig = 0;

// special cases include:
// y = 0, +inf, -inf, NaN
//
//      x	|       +inf	|	-inf	|       NaN     
// =============================================================
// 	exp(x)	|	+inf	|	0	|	NaN	
//
    bool x_is_NaN = 0;
    bool x_is_inf = 0;
    bool x_is_pinf = 0;
    bool x_is_ninf = 0;

    if ( ::hlstmp::__isnan(x) )    x_is_NaN = 1;
    if ( ::hlstmp::__isinf(x) )	x_is_inf = 1;
    x_is_pinf = x_is_inf & ~es.sign[0];
    x_is_ninf = x_is_inf & es.sign[0];

    if (x_is_NaN) {
        // out = NaN
        //out.sign[0] = 0;
        out.sig = -1; // all 1's
        out.exp = fp_struct<T>::EXP_INFNAN;
        return out.to_ieee();
    }
    if (x_is_pinf) {
	// out = +inf
	//out.sign[0] = 0;
	//out.sig = 0;
	out.exp = fp_struct<T>::EXP_INFNAN;
	return out.to_ieee();
    }
    if (x_is_ninf) {
        // out = 0
        //out.sign[0] = 0;
        //out.sig = 0;
        out.exp = 0;
        return out.to_ieee();
    }


    int m_exp = es.expv();
    fp_struct<T> nes = es;
#ifdef STDSUBNORMALS
    if(nes.exp == 0 && nes.sig != 0) {
        // subnormal handling.
        unsigned int zeros;
        #pragma unroll
        for (zeros = 0; zeros < wf; zeros++)
            if ( nes.sig[wf-zeros-1] == 1 ) break;
        m_exp -= zeros;
        nes.sig = nes.sig << (zeros + 1); // add one so we shift off the leading one
    }
#endif
    ap_fixed<1 + 1 + wf, 1 + 1> e_frac = 0;
    e_frac[e_frac.wl()-e_frac.iwl()] = 1; // The implicit '1' in IEEE format.
    e_frac(e_frac.wl()-e_frac.iwl()-1,0) = nes.sig(wf-1,0);
    if (nes.sign) e_frac = -e_frac;

    const static int gbits = exp_traits<T>::gbits;
    ap_fixed<1 + we + wf+gbits, 1 + we> m_frac_l = e_frac;

    ap_fixed<1 + we + wf, 1 + we> m_fix_l = m_frac_l << m_exp; // used for overflow checking only
    ap_fixed<1 + we + wf, 1 + we> m_fix_back = m_fix_l >> m_exp;

    ap_fixed<1 + we + wf+gbits, 1 + we> m_fix;
    m_fix = m_frac_l << m_exp;

    ap_fixed<1 + we + 4, 1 + we> m_fix_hi = m_fix;

    const ap_ufixed<1 + we+3, 1> LOG2R = 1.4426950408889634073599246810019;

    ap_fixed<2, 1> delta1;
    delta1[1] = m_fix[m_fix.wl()-1];
    delta1[0] = 1;

    ap_int<1 + we+1> r_exp = m_fix_hi * LOG2R + delta1;

    const ap_ufixed<wf+gbits+we+1, 0> LOG2_hi = 0.69314718055994517520446152047953;
    const ap_ufixed<wf+gbits+we+1, 0> LOG2_lo = 0.60444058366692929317548838826088;
    const ap_ufixed<wf+gbits+we+1, 0> LOG2 = LOG2_hi + ( LOG2_lo >> 52 );

    ap_fixed<1 + we + wf+gbits, 1 + we+1> m_fix_a = r_exp * LOG2;// m_fix approximation

    assert( (m_fix_back != m_frac_l) || (m_fix - m_fix_a < 0.5) );// check r_exp zeros out integer and most significant fraction bits
    assert( (m_fix_back != m_frac_l) || (m_fix - m_fix_a > -0.5) );// check r_exp zeros out integer and most significant fraction bits
    ap_fixed<1 -1 + wf+gbits, 1 -1> m_diff = m_fix - m_fix_a;

    // e^Y = 1 + Y + Y^2/2 + ... + Y^n/n! + ...
    // term Y^n/n! can be eliminated when its MSB is less than 2^-(wf+g)
    // Y belongs to (-.5,.5)
    // w = wf+g
    // g = 3,4,7 for h,f,d
    // g_Z2 = 3,3,5 for h,f,d
    // Y = Z1 + Z1P
    //   = Z1 + Z2 + Z2P
    //   = ...
    //   = Z1 + Z2 + ... + Zk
    // wn is width of Zn, n = 1...k
    // T_Z1 = 2^w1*(w+1)
    // T_Z2 = 2^w2*(w+1-2*w1)
    // T_Z3 = 2^w3*(w+1-2*(w1+w2))
    // ...
    //
    //		|	h	|	f	|	d	
    //	========================================================
    //	wf	|	10	|	23	|	52	
    //	g	|	3	|	4	|	7	
    //	g_Z2    |       3       |       3       |       5
    //	w	|	13	|	27	|	59	
    //	k	|	2	|	2	|	4	
    //	wn	|	9,4	|	9,18	|    8,8,8,35	
    //	T_total	|	7k	|	<18k	| 14.5k+10.5k+6.5k
    //	Mult	|	5bit	|	1DSP	|	16DSP	

    const static int w_Z1 = exp_traits<T>::w_Z1;
    // Z1
    ap_uint<w_Z1> m_diff_hi = m_diff ( m_diff.wl()-1 , m_diff.wl()-w_Z1 );
    // Z1P = Z2 + ... + Zk
    ap_ufixed<wf+gbits-w_Z1, -w_Z1> m_diff_lo = m_diff; // ( m_diff.wl()-m_diff.iwl()-w_Z1-1 , 0 );

    // e^Z1 by table_exp_Z1
    const static int gbits_Z2 = exp_traits<T>::gbits_Z2;
    ap_ufixed<1 + wf+gbits_Z2, 1> exp_Z1 = table_exp_Z1< ap_ufixed<1 + wf+gbits_Z2, 1> >::array [ m_diff_hi ];
    ap_ufixed<wf+gbits_Z2-w_Z1+1, -w_Z1+1> exp_Z1P_m_1 = exp_traits<T>::exp_Z1P_m_1 ( m_diff_lo );
    ap_ufixed<1 + wf+gbits_Z2-w_Z1, 1> exp_Z1_hi = exp_Z1;
    ap_ufixed<1, -wf> delta;
    delta[0] = 1;
    ap_ufixed<2 + wf+gbits_Z2, 2> exp_Y_l = ( exp_Z1 + delta ) + exp_Z1_hi * exp_Z1P_m_1;
    assert ( exp_Y_l[exp_Y_l.wl()-1] == 0 );
    ap_ufixed<1 + wf+gbits_Z2, 1> exp_Y = exp_Y_l;

    if ( exp_Y[exp_Y.wl()-1] == 0 ) {
        exp_Y = exp_Y << 1;
        r_exp = r_exp - 1;
    }

    // check overflow here
    if ( ( m_exp > 0 && m_fix_back != m_frac_l ) || ( r_exp > fp_struct<T>::EXP_BIAS ) ) {
	if ( ~m_frac_l[m_frac_l.wl()-1] ) {
	// out = +inf
	//out.sign[0] = 0;
	//out.sig = 0;
            out.exp = fp_struct<T>::EXP_INFNAN;
            return out.to_ieee();
	} else {
	// out = 0
	//out.sign[0] = 0;
	//out.sig = 0;
            out.exp = 0;
            return out.to_ieee();
	}
    }

    // check underflow here
    if ( r_exp <= -fp_struct<T>::EXP_BIAS ) {
        // out = 0
        //out.sign[0] = 0;
        //out.sig = 0;
        out.exp = 0;
        return out.to_ieee();
    }

// let's output the result
    out.sig(wf-1,0) = exp_Y ( exp_Y.wl()-1-1 , exp_Y.wl()-1-wf );
    out.exp = fp_struct<T>::EXP_BIAS+r_exp;
    return out.to_ieee();

}

static half exp(half x)
{
        return exp_generic(x);
}
static float exp(float x)
{
        return exp_generic(x);
}
static double exp(double x)
{
        return exp_generic(x);
}

static float expf(float x)
{
        return exp_generic(x);
}

static half half_exp(half x)
{
        return exp_generic(x);
}


template<int W_, int I_>
ap_fixed<W_,I_> exp(ap_fixed<W_,I_> x)
{
#pragma HLS pipeline

    int F_ = W_ - I_;
    if (I_>33) return 0;
    else if (F_>32) return 0;

    // I_s_ is the number of integer bits we need to calculate
    // I_s_ = ceil( ln2 * max ( I_-1 , F_ ) )
    // I_s_ = [1..6]
    int I_s_;
    int M_ = (I_-1>F_) ? (I_-1) : F_;
    if (M_==1)          I_s_ = 1;
    else if (M_==2)     I_s_ = 2;
    else if (M_<6)      I_s_ = 3;
    else if (M_<12)     I_s_ = 4;
    else if (M_<24)     I_s_ = 5;
    else                I_s_ = 6;

    ap_fixed<W_,I_> r;
    if (I_s_==1) { // I_s_ = 1, M_ = 1, I_ <= 2, F_ <= 1, W <= 3
        ap_fixed<3,2> x_l = x;
        ap_ufixed<2,1> y = 0;
        if (x_l[2]!=x_l[1]) { // overflow
            if (!x_l[2]) y = "0x1.8p0";
        } else {
            if (!x_l[2]) y[1] = 1;
            if (x_l[2]|x_l[0]) y[0] = 1;
        }
        if (I_<2) { // overflow
            if (y[1]) {
                y[1] = 0;
                y[0] = 1;
            }
        }
        r = y;
    } else if (I_s_==2) { // I_s_ = 2, M_ = 2, I_ <= 3, F_ <= 2, W <= 5
        ap_fixed<5,3> x_l = x;
        ap_ufixed<4,2> y = 0;
        ap_fixed<2,2> x_l_int;
        x_l_int(1,0) = x_l(3,2);
        ap_ufixed<2,0> x_l_fract;
        x_l_fract(1,0) = x_l(1,0);
        //if ((x_l[4]!=x_l[3])||(x_l>="0x1.8p0")) { // overflow
        if ((x_l[4]!=x_l[3])||((x_l_int==1)&&(x_l_fract>="0x0.8p0"))) { // overflow
            if (!x_l[4]) y = "0x3.cp0";
        } else {
            if (!x_l[4]&((x_l[1]&x_l[0])|x_l[2])) y[3] = 1;
            if (!x_l[4]&((!x_l[2]&!x_l[0])|(!x_l[1]&x_l[0]))) y[2] = 1;
            if ((!x_l[4]&x_l[1]&!x_l[0])|(!x_l[4]&x_l[2])|(x_l[2]&x_l[0])|(x_l[2]&x_l[1])) y[1] = 1;
            if ((!x_l[2]&!x_l[1]&x_l[0])|(!x_l[2]&x_l[1]&!x_l[0])|(x_l[2]&!x_l[1]&!x_l[0])|(x_l[2]&x_l[1]&x_l[0])|(x_l[4]&!x_l[3])) y[0] = 1;
        }
        if (I_<3) { // overflow
            bool overf = 0;
            #pragma unroll
            for (int j = 3; j >= I_+1; j--) {
                if (y[j])
                    overf = 1;
            }
            if (overf) {
                #pragma unroll
                for (int j = 3; j >= I_+1; j--)
                    y[j] = 0;
                #pragma unroll
                for (int j = I_; j >= 0; j--)
                    y[j] = 1;
            }
        }
        r = y;
    } else if (I_s_==3) { // I_s_ = 3, M_ = 3..5, I_ <= 6, F_ <= 5, W <= 11
        ap_fixed<11,6> x_l = x;
        ap_ufixed<10,5> y = 0;
        ap_fixed<3,3> x_l_int;
        x_l_int(2,0) = x_l(7,5);
        ap_ufixed<5,0> x_l_fract;
        x_l_fract(4,0) = x_l(4,0);
        //if ((x_l[10]!=x_l[9])||(x_l[10]!=x_l[8])||(x_l[10]!=x_l[7])||(x_l>"0x3.7p0")) { // overflow
        if ((x_l[10]!=x_l[9])||(x_l[10]!=x_l[8])||(x_l[10]!=x_l[7])||((x_l_int==3)&&(x_l_fract>"0x0.7p0"))) { // overflow
            if (!x_l[10]) y = "0x1f.f8p0";
        } else {
//            ap_uint<6> x_msb_ind;
//            x_msb_ind[5] = x_l[10];
//            x_msb_ind(4,0) = x_l(6,2);
//            ap_ufixed<2,-3> x_lsb = x_l;

            ap_uint<4> x_msb_ind;
            x_msb_ind[3] = x_l[10];
            x_msb_ind(2,0) = x_l(6,4);
            ap_uint<4> x_lsb_ind;
            x_lsb_ind(3,0) = x_l(3,0);
            ap_ufixed<4,-1> x_lsb = x_l;

//            const ap_ufixed<11,5> exp_x_msb_table[64] = {
////                0, 0, 0, 0, 29.21875, 25.796875, 22.765625, 20.078125, 
////                17.71875, 15.640625, 13.796875, 12.1875, 10.75, 9.484375, 8.375, 7.390625, 
////                6.515625, 5.75, 5.078125, 4.484375, 3.953125, 3.484375, 3.078125, 2.71875, 
////                2.40625, 2.109375, 1.875, 1.65625, 1.453125, 1.28125, 1.140625, 1, 
////                0.015625, 0.015625, 0.03125, 0.03125, 0.03125, 0.03125, 0.03125, 0.046875, 
////                0.046875, 0.0625, 0.0625, 0.078125, 0.078125, 0.09375, 0.109375, 0.125, 
////                0.140625, 0.15625, 0.171875, 0.203125, 0.21875, 0.25, 0.28125, 0.328125, 
////                0.375, 0.421875, 0.46875, 0.53125, 0.609375, 0.6875, 0.78125, 0.875, 
//                1, 1.140625, 1.28125, 1.453125, 1.65625, 1.875, 2.109375, 2.40625,
//                2.71875, 3.078125, 3.484375, 3.953125, 4.484375, 5.078125, 5.75, 6.515625,
//                7.390625, 8.375, 9.484375, 10.75, 12.1875, 13.796875, 15.640625, 17.71875,
//                20.078125, 22.765625, 25.796875, 29.21875, 33.109375, 37.53125, 42.515625, 48.1875,
//                0.015625, 0.015625, 0.03125, 0.03125, 0.03125, 0.03125, 0.03125, 0.046875,
//                0.046875, 0.0625, 0.0625, 0.078125, 0.078125, 0.09375, 0.109375, 0.125,
//                0.140625, 0.15625, 0.171875, 0.203125, 0.21875, 0.25, 0.28125, 0.328125,
//                0.375, 0.421875, 0.46875, 0.53125, 0.609375, 0.6875, 0.78125, 0.875,
//            };

            const ap_ufixed<12,0> exp_x_lsb_m_1_table[16] = {
                0, "0x0.082p0", "0x0.108p0", "0x0.193p0", "0x0.221p0", "0x0.2b5p0", "0x0.34dp0", "0x0.3eap0",
                "0x0.48bp0", "0x0.532p0", "0x0.5dfp0", "0x0.69p0", "0x0.748p0", "0x0.805p0", "0x0.8c8p0", "0x0.991p0",
            };
            ap_ufixed<12, 0> exp_x_lsb_m_1 = exp_x_lsb_m_1_table[x_lsb_ind];

            const ap_ufixed<12,5> exp_x_msb_table[16] = {
                1, "0x1.a6p0", "0x2.b8p0", "0x4.7cp0", "0x7.64p0", "0xc.2ep0", "0x14.16p0", "0x21.1ep0",
                "0x0.04p0", "0x0.08p0", "0x0.0cp0", "0x0.16p0", "0x0.22p0", "0x0.3ap0", "0x0.5ep0", "0x0.9cp0",
            };
            ap_ufixed<12,5> exp_x_msb = exp_x_msb_table[x_msb_ind];
            ap_ufixed<24,5> y_lo = exp_x_msb * exp_x_lsb_m_1;
            ap_ufixed<12,5> y_lo_s = y_lo;
            ap_ufixed<12,5> y_l = y_lo_s + exp_x_msb;
            y = y_l;
        }
        if (I_<6) { // overflow
            bool overf = 0;
            #pragma unroll
            for (int j = 9; j >= I_+4; j--) {
                if (y[j])
                    overf = 1;
            }
            if (overf) {
                #pragma unroll
                for (int j = 9; j >= I_+4; j--)
                    y[j] = 0;
                #pragma unroll
                for (int j = I_+3; j >= 0; j--)
                    y[j] = 1;
            }
        }
        r = y;
    } else if (I_s_==4) { // I_s_ = 4, M_ = 6..11, I_ <= 12, F_ <= 11, W <= 23
        ap_fixed<23,12> x_l = x;
        ap_ufixed<22,11> y = 0;
        ap_fixed<4,4> x_l_int;
        x_l_int(3,0) = x_l(14,11);
        ap_ufixed<11,0> x_l_fract;
        x_l_fract(10,0) = x_l(10,0);
        bool overf = 0;
        #pragma unroll
        for (int j = 14; j < 22; j++)
            if (x_l[22]!=x_l[j])
                overf = 1;
        //if (overf||(x_l>"0x7.ap0")) { // overflow
        if (overf||((x_l_int==7)&&(x_l_fract>"0x0.ap0"))) { // overflow
            if (!x_l[22]) {
                #pragma unroll
                for (int j = 0; j < 21; j++) y[j] = 1;
            }
        } else {
            ap_uint<5> x_msb_ind_1;
            x_msb_ind_1[4] = x_l[22];
            x_msb_ind_1(3,0) = x_l(13,10);
            ap_uint<5> x_msb_ind_2;
            x_msb_ind_2(4,0) = x_l(9,5);
            ap_ufixed<5,-1> x_msb_2 = x_l;
            ap_uint<5> x_lsb_ind;
            x_lsb_ind(4,0) = x_l(4,0);
            //ap_ufixed<5,-6> x_lsb = x_l;

            const ap_ufixed<11,-13> f_x_lsb_table[32] = {
                0, "0x0.000002p0", "0x0.000008p0", "0x0.000012p0",
                "0x0.00002p0", "0x0.000032p0", "0x0.000048p0", "0x0.000062p0",
                "0x0.00008p0", "0x0.0000a2p0", "0x0.0000c9p0", "0x0.0000f3p0",
                "0x0.000121p0", "0x0.000153p0", "0x0.000189p0", "0x0.0001c3p0",
                "0x0.000202p0", "0x0.000244p0", "0x0.00028ap0", "0x0.0002d4p0",
                "0x0.000323p0", "0x0.000375p0", "0x0.0003ccp0", "0x0.000426p0",
                "0x0.000485p0", "0x0.0004e7p0", "0x0.00054ep0", "0x0.0005b9p0",
                "0x0.000627p0", "0x0.00069ap0", "0x0.000711p0", "0x0.00078cp0",
            };
            ap_ufixed<11,-13> f_x_lsb = f_x_lsb_table[x_lsb_ind];
            ap_ufixed<18, -6> exp_x_lsb_m_1 = 0;
            exp_x_lsb_m_1(17,13) = x_lsb_ind(4,0);
            exp_x_lsb_m_1(10,0)  = f_x_lsb(10,0);

            const ap_ufixed<25,0> exp_x_msb_2_m_1_table[32] = {
                "0x0.0000000p0", "0x0.04080a8p0", "0x0.0820560p0", "0x0.0c49238p0", 
                "0x0.1082b58p0", "0x0.14cd500p0", "0x0.1929370p0", "0x0.1d96b10p0", 
                "0x0.2216048p0", "0x0.26a7790p0", "0x0.2b4b588p0", "0x0.3001ed0p0", 
                "0x0.34cb818p0", "0x0.39a8628p0", "0x0.3e98de8p0", "0x0.439d440p0", 
                "0x0.48b5e40p0", "0x0.4de30f0p0", "0x0.5325180p0", "0x0.587c540p0", 
                "0x0.5de9178p0", "0x0.636bb98p0", "0x0.6904930p0", "0x0.6eb3fc8p0", 
                "0x0.747a510p0", "0x0.7a57ee0p0", "0x0.804d300p0", "0x0.865a778p0", 
                "0x0.8c80248p0", "0x0.92be998p0", "0x0.99163b0p0", "0x0.9f876e8p0", 
            };
            ap_ufixed<25, 0> exp_x_msb_2_m_1 = exp_x_msb_2_m_1_table[x_msb_ind_2];

            ap_ufixed<43,-6> f_x_msb_2_lsb = exp_x_msb_2_m_1 * exp_x_lsb_m_1;
            ap_ufixed<19,-6> f_x_msb_2_lsb_s = f_x_msb_2_lsb;
            ap_ufixed<25, 0> exp_x_msb_2_lsb_m_1 = f_x_msb_2_lsb_s + exp_x_lsb_m_1 + exp_x_msb_2_m_1;

            const ap_ufixed<25,11> exp_x_msb_1_table[32] = {
                "0x1.0000p0", "0x1.a614p0", "0x2.b7e0p0", "0x4.7b50p0", 
                "0x7.6398p0", "0xc.2eb8p0", "0x14.15e4p0", "0x21.1d90p0", 
                "0x36.9920p0", "0x5a.0464p0", "0x94.69c4p0", "0xf4.b124p0", 
                "0x193.6dc4p0", "0x299.2444p0", "0x448.a218p0", "0x710.0adcp0", 
                "0x0.0014p0", "0x0.0024p0", "0x0.003cp0", "0x0.0064p0", 
                "0x0.00a4p0", "0x0.010cp0", "0x0.01b8p0", "0x0.02d8p0", 
                "0x0.04b0p0", "0x0.07bcp0", "0x0.0cc0p0", "0x0.1504p0", 
                "0x0.22a4p0", "0x0.3920p0", "0x0.5e2cp0", "0x0.9b44p0", 
            };
            ap_ufixed<25,11> exp_x_msb_1 = exp_x_msb_1_table[x_msb_ind_1];
            ap_ufixed<50,11> y_lo = exp_x_msb_1 * exp_x_msb_2_lsb_m_1;
            ap_ufixed<25,11> y_lo_s = y_lo;
            ap_ufixed<25,11> y_l = y_lo_s + exp_x_msb_1;
            y = y_l;
        }
        if (I_<12) { // overflow
            bool overf = 0;
            #pragma unroll
            for (int j = 21; j >= I_+10; j--) {
                if (y[j])
                    overf = 1;
            }
            if (overf) {
                #pragma unroll
                for (int j = 21; j >= I_+10; j--)
                    y[j] = 0;
                #pragma unroll
                for (int j = I_+9; j >= 0; j--)
                    y[j] = 1;
            }
        }
        r = y;
    } else if (I_s_==5) { // I_s_ = 5, M_ = 12..23, I_ <= 24, F_ <= 23, W <= 47
        ap_fixed<47,24> x_l = x;
        ap_ufixed<46,23> y = 0;
        ap_fixed<5,5> x_l_int;
        x_l_int(4,0) = x_l(27,23);
        ap_ufixed<23,0> x_l_fract;
        x_l_fract(22,0) = x_l(22,0);
        bool overf = 0;
        #pragma unroll
        for (int j = 27; j < 46; j++)
            if (x_l[46]!=x_l[j])
                overf = 1;
        //if (overf||(x_l>="0xf.f14028p0")) { // overflow
        if (overf||((x_l_int==15)&&(x_l_fract>="0x0.f14028p0"))) { // overflow
            if (!x_l[46]) {
                #pragma unroll
                for (int j = 0; j < 45; j++) y[j] = 1;
            }
        } else {
            ap_uint<8> x_msb_ind_1;
            x_msb_ind_1[7] = x_l[46];
            x_msb_ind_1(6,0) = x_l(26,20);
            ap_uint<8> x_msb_ind_2;
            x_msb_ind_2(7,0) = x_l(19,12);
            ap_uint<5> x_msb_ind_3;
            x_msb_ind_3(4,0) = x_l(11,7);
            ap_uint<3> x_msb_ind_4;
            x_msb_ind_4(2,0) = x_l(6,4);
            ap_uint<4> x_lsb_ind;
            x_lsb_ind(3,0) = x_l(3,0);

            ap_ufixed<8, -3> x_msb_2 = x_l;
            ap_ufixed<5,-11> x_msb_3 = x_l;
            ap_ufixed<3,-16> x_msb_4 = x_l;
            ap_ufixed<4,-19> x_lsb   = x_l;

            const ap_ufixed< 6,-33> f_x_msb_4_h_table[8] = {
"0x0.0000000000p0", "0x0.0000000002p0", "0x0.0000000008p0", "0x0.0000000012p0",
"0x0.0000000020p0", "0x0.0000000032p0", "0x0.0000000048p0", "0x0.0000000062p0",
            };
            const ap_ufixed< 7,-51> f_x_msb_4_l_table[8] = {
"0x0.000000000000000p0", "0x0.000000000000000p0", "0x0.00000000000000cp0", "0x0.000000000000024p0",
"0x0.000000000000054p0", "0x0.0000000000000a8p0", "0x0.000000000000120p0", "0x0.0000000000001c8p0",
            };
            ap_ufixed< 6,-33> f_x_msb_4_h = f_x_msb_4_h_table[x_msb_ind_4];
            ap_ufixed< 7,-51> f_x_msb_4_l = f_x_msb_4_l_table[x_msb_ind_4];

            const ap_ufixed< 8,-39> f_x_lsb_table[16] = {
"0x0.000000000000p0", "0x0.000000000002p0", "0x0.000000000008p0", "0x0.000000000012p0",
"0x0.000000000020p0", "0x0.000000000032p0", "0x0.000000000048p0", "0x0.000000000062p0",
"0x0.000000000080p0", "0x0.0000000000a2p0", "0x0.0000000000c8p0", "0x0.0000000000f2p0", 
"0x0.000000000120p0", "0x0.000000000152p0", "0x0.000000000188p0", "0x0.0000000001c2p0",
            };
            ap_ufixed< 8,-39> f_x_lsb = f_x_lsb_table[x_lsb_ind];

            ap_ufixed< 7,-35> f_x_msb_4_lsb = x_msb_4 * x_lsb;

            ap_ufixed<10,-32> exp_x_msb_4_lsb_m_1_m = 0;
            exp_x_msb_4_lsb_m_1_m(8,3) = f_x_msb_4_h(5,0);
            exp_x_msb_4_lsb_m_1_m(2,0) = f_x_lsb(7,5);
            exp_x_msb_4_lsb_m_1_m = exp_x_msb_4_lsb_m_1_m + f_x_msb_4_lsb;

            ap_ufixed<42,-16> exp_x_msb_4_lsb_m_1 = 0;
            exp_x_msb_4_lsb_m_1(41,35) = x_l(6,0);
            exp_x_msb_4_lsb_m_1(25,16) = exp_x_msb_4_lsb_m_1_m(9,0);
            exp_x_msb_4_lsb_m_1(15,11) = f_x_lsb(4,0);
            exp_x_msb_4_lsb_m_1( 6, 0) = f_x_msb_4_l(6,0);


            const ap_ufixed<32,-23> f_x_msb_3_table[32] = {
"0x0.00000000000000p0", "0x0.0000000080002ap0", "0x0.00000002000156p0", "0x0.00000004800480p0", 
"0x0.00000008000aaap0", "0x0.0000000c8014d6p0", "0x0.00000012002400p0", "0x0.0000001880392cp0", 
"0x0.00000020005556p0", "0x0.00000028807982p0", "0x0.0000003200a6acp0", "0x0.0000003c80ddd8p0", 
"0x0.00000048012004p0", "0x0.00000054816e30p0", "0x0.0000006201c95cp0", "0x0.00000070823288p0", 
"0x0.0000008002aab6p0", "0x0.000000908332e2p0", "0x0.000000a203cc12p0", "0x0.000000b4847740p0", 
"0x0.000000c8053570p0", "0x0.000000dc8607a0p0", "0x0.000000f206eed0p0", "0x0.0000010887ec02p0", 
"0x0.00000120090036p0", "0x0.000001388a2c6ap0", "0x0.000001520b71a0p0", "0x0.0000016c8cd0d6p0", 
"0x0.000001880e4b0ep0", "0x0.000001a48fe148p0", "0x0.000001c2119484p0", "0x0.000001e09365c0p0", 
            };

            ap_ufixed<32,-23> f_x_msb_3 = f_x_msb_3_table[x_msb_ind_3];
            ap_ufixed<44,-11> exp_x_msb_3_m_1 = 0;
            exp_x_msb_3_m_1(43,39) = x_msb_3(4,0);
            exp_x_msb_3_m_1(31, 0) = f_x_msb_3(31,0);

            ap_ufixed<86,-27> f_x_msb_3_4_lsb = exp_x_msb_3_m_1 * exp_x_msb_4_lsb_m_1;
            ap_ufixed<31,-27> f_x_msb_3_4_lsb_s = f_x_msb_3_4_lsb;
            ap_ufixed<48,-10> exp_x_msb_3_4_lsb_m_1 = f_x_msb_3_4_lsb_s + exp_x_msb_3_m_1 + exp_x_msb_4_lsb_m_1;


            const ap_ufixed<46,-6> f_x_msb_2_table[256] = {
"0x0", "0x0.0000020015560p0", "0x0.00000800AAB55p0", "0x0.0000120240360p0", 
"0x0.0000200556001p0", "0x0.0000320A6C4B8p0", "0x0.0000481203608p0", "0x0.0000621C9B971p0", 
"0x0.0000802AB5577p0", "0x0.0000A23CD119Dp0", "0x0.0000C8536F668p0", "0x0.0000F26F10D5Dp0", 
"0x0.0001209036103p0", "0x0.000152B75FCE3p0", "0x0.000188E50ED86p0", "0x0.0001C319C4077p0", 
"0x0.0002015600445p0", "0x0.0002439A4487Ep0", "0x0.000289E711DB3p0", "0x0.0002D43CE9577p0", 
"0x0.0003229C4C260p0", "0x0.00037505BB805p0", "0x0.0003CB79B8B01p0", "0x0.000425F8C50F2p0", 
"0x0.0004848362076p0", "0x0.0004E71A11131p0", "0x0.00054DBD53BC8p0", "0x0.0005B86DAB9E3p0", 
"0x0.0006272B9A630p0", "0x0.000699F7A1C5Dp0", "0x0.000710D24391Ep0", "0x0.00078BBC01A29p0", 
"0x0.00080AB55DE39p0", "0x0.00088DBEDA50Bp0", "0x0.000914D8F8F63p0", "0x0.0009A0043BF07p0", 
"0x0.000A2F41256C2p0", "0x0.000AC29037A63p0", "0x0.000B59F1F4EBDp0", "0x0.000BF566DF9AAp0", 
"0x0.000C94EF7A206p0", "0x0.000D388C46FB4p0", "0x0.000DE03DC8B9Bp0", "0x0.000E8C0481FA7p0", 
"0x0.000F3BE0F56CAp0", "0x0.000FEFD3A5CFAp0", "0x0.0010A7DD15F36p0", "0x0.001163FDC8B7Fp0", 
"0x0.00122436410DDp0", "0x0.0012E88701F5Ep0", "0x0.0013B0F08E817p0", "0x0.00147D7369D22p0", 
"0x0.00154E101719Fp0", "0x0.001622C7199B7p0", "0x0.0016FB98F4A96p0", "0x0.0017D8862BA72p0", 
"0x0.0018B98F42084p0", "0x0.00199EB4BB511p0", "0x0.001A87F71B161p0", "0x0.001B7556E4FC4p0", 
"0x0.001C66D49CB93p0", "0x0.001D5C70C612Dp0", "0x0.001E562BE4DFAp0", "0x0.001F54067D067p0", 
"0x0.00205601127ECp0", "0x0.00215C1C29507p0", "0x0.0022665845940p0", "0x0.002374B5EB724p0", 
"0x0.002487359F24Cp0", "0x0.00259DD7E4F57p0", "0x0.0026B89D413EFp0", "0x0.0027D786386C4p0", 
"0x0.0028FA934EF90p0", "0x0.002A21C509717p0", "0x0.002B4D1BEC726p0", "0x0.002C7C987CA92p0", 
"0x0.002DB03B3ED3Ap0", "0x0.002EE804B7C07p0", "0x0.003023F56C4ECp0", "0x0.0031640DE16E3p0", 
"0x0.0032A84E9C1F5p0", "0x0.0033F0B821730p0", "0x0.00353D4AF68B0p0", "0x0.00368E07A0999p0", 
"0x0.0037E2EEA4E19p0", "0x0.00393C0088B6Cp0", "0x0.003A993DD17D5p0", "0x0.003BFAA704AA3p0", 
"0x0.003D603CA7C32p0", "0x0.003EC9FF405E6p0", "0x0.004037EF54230p0", "0x0.0041AA0D68C8Cp0", 
"0x0.0043205A04182p0", "0x0.00449AD5ABEA5p0", "0x0.00461980E6294p0", "0x0.00479C5C38CF9p0", 
"0x0.0049236829E8Bp0", "0x0.004AAEA53F90Ep0", "0x0.004C3E13FFF51p0", "0x0.004DD1B4F152Fp0", 
"0x0.004F698899F90p0", "0x0.0051058F8046Ap0", "0x0.0052A5CA2AABFp0", "0x0.00544A391FA9Cp0", 
"0x0.0055F2DCE5D1Ep0", "0x0.00579FB603C6Ep0", "0x0.005950C5003C2p0", "0x0.005B060A61F5Dp0", 
"0x0.005CBF86AFC91p0", "0x0.005E7D3A709BEp0", "0x0.00603F262B650p0", "0x0.0062054A672C2p0", 
"0x0.0063CFA7AB09Dp0", "0x0.00659E3E7E278p0", "0x0.0067710F67BFAp0", "0x0.0069481AEF1D5p0", 
"0x0.006B23619B9CFp0", "0x0.006D02E3F4AB7p0", "0x0.006EE6A281C6Fp0", "0x0.0070CE9DCA7E6p0", 
"0x0.0072BAD65671Bp0", "0x0.0074AB4CAD51Cp0", "0x0.0076A00156E07p0", "0x0.007898F4DAF09p0", 
"0x0.007A9627C165Fp0", "0x0.007C979A92356p0", "0x0.007E9D4DD564Ap0", "0x0.0080A742130A8p0", 
"0x0.0082B577D34EDp0", "0x0.0084C7EF9E6A7p0", "0x0.0086DEA9FCA73p0", "0x0.0088F9A776601p0", 
"0x0.008B18E894010p0", "0x0.008D3C6DDE06Fp0", "0x0.008F6437DD000p0", "0x0.00919047198B6p0", 
"0x0.0093C09C1C595p0", "0x0.0095F5376E2B2p0", "0x0.00982E1997D33p0", "0x0.009A6B4322352p0", 
"0x0.009CACB496458p0", "0x0.009EF26E7D0A2p0", "0x0.00A13C715F99Ep0", "0x0.00A38ABDC71CEp0", 
"0x0.00A5DD543CCC4p0", "0x0.00A8343549F26p0", "0x0.00AA8F6177EADp0", "0x0.00ACEED950222p0", 
"0x0.00AF529D5C165p0", "0x0.00B1BAAE25566p0", "0x0.00B4270C3582Ap0", "0x0.00B697B8164C7p0", 
"0x0.00B90CB25176Ap0", "0x0.00BB85FB70D50p0", "0x0.00BE0393FE4CCp0", "0x0.00C0857C83D44p0", 
"0x0.00C30BB58B732p0", "0x0.00C5963F9F424p0", "0x0.00C8251B496BEp0", "0x0.00CAB849142B5p0", 
"0x0.00CD4FC989CD6p0", "0x0.00CFEB9D34B00p0", "0x0.00D28BC49F428p0", "0x0.00D5304054059p0", 
"0x0.00D7D910DD8B1p0", "0x0.00DA8636C6764p0", "0x0.00DD37B2997BCp0", "0x0.00DFED84E1618p0", 
"0x0.00E2A7AE28FECp0", "0x0.00E5662EFB3C3p0", "0x0.00E82907E313Dp0", "0x0.00EAF0396B910p0", 
"0x0.00EDBBC41FD08p0", "0x0.00F08BA88B009p0", "0x0.00F35FE73860Bp0", "0x0.00F63880B341Ep0", 
"0x0.00F9157587069p0", "0x0.00FBF6C63F228p0", "0x0.00FEDC73671B0p0", "0x0.0101C67D8A86Cp0", 
"0x0.0104B4E5350DFp0", "0x0.0107A7AAF26A2p0", "0x0.010A9ECF4E667p0", "0x0.010D9A52D4DF8p0", 
"0x0.01109A3611C34p0", "0x0.01139E7991116p0", "0x0.0116A71DDEDADp0", "0x0.0119B42387423p0", 
"0x0.011CC58B167B9p0", "0x0.011FDB5518CCAp0", "0x0.0122F5821A8C7p0", "0x0.01261412A823Cp0", 
"0x0.012937074E0CDp0", "0x0.012C5E6098D37p0", "0x0.012F8A1F15151p0", "0x0.0132BA434F80Ap0", 
"0x0.0135EECDD4D6Cp0", "0x0.013927BF31E98p0", "0x0.013C6517F39CCp0", "0x0.013FA6D8A6E5Dp0", 
"0x0.0142ED01D8CBCp0", "0x0.0146379416673p0", "0x0.0149868FECE26p0", "0x0.014CD9F5E9795p0", 
"0x0.015031C699799p0", "0x0.01538E028A426p0", "0x0.0156EEAA4944Bp0", "0x0.015A53BE64033p0", 
"0x0.015DBD3F68122p0", "0x0.01612B2DE3178p0", "0x0.01649D8A62CB1p0", "0x0.0168145574F65p0", 
"0x0.016B8F8FA7745p0", "0x0.016F0F3988321p0", "0x0.01729353A52E4p0", "0x0.01761BDE8C792p0", 
"0x0.0179A8DACC350p0", "0x0.017D3A48F295Dp0", "0x0.0180D0298DE13p0", "0x0.01846A7D2C6ECp0", 
"0x0.018809445CA7Bp0", "0x0.018BAC7FAD074p0", "0x0.018F542FAC1A4p0", "0x0.01930054E87F8p0", 
"0x0.0196B0EFF0E79p0", "0x0.019A66015414Dp0", "0x0.019E1F89A0DBAp0", "0x0.01A1DD8966221p0", 
"0x0.01A5A00132E02p0", "0x0.01A966F1961FCp0", "0x0.01AD325B1EFC9p0", "0x0.01B1023E5CA45p0", 
"0x0.01B4D69BDE569p0", "0x0.01B8AF743364Cp0", "0x0.01BC8CC7EB323p0", "0x0.01C06E9795345p0", 
"0x0.01C454E3C0F24p0", "0x0.01C83FACFE054p0", "0x0.01CC2EF3DC187p0", "0x0.01D022B8EAE8Fp0", 
"0x0.01D41AFCBA45Ep0", "0x0.01D817BFDA104p0", "0x0.01DC1902DA3B1p0", "0x0.01E01EC64ACB7p0", 
"0x0.01E4290ABBD87p0", "0x0.01E837D0BD8B2p0", "0x0.01EC4B18E01E9p0", "0x0.01F062E3B3DFEp0", 
"0x0.01F47F31C92E4p0", "0x0.01F8A003B07AFp0", "0x0.01FCC559FA492p0", "0x0.0200EF35372E4p0", 
"0x0.02051D95F7D1Cp0", "0x0.0209507CCCED1p0", "0x0.020D87EA474BEp0", "0x0.0211C3DEF7CBDp0", 
            };
            ap_ufixed<46,-6> f_x_msb_2 = f_x_msb_2_table[x_msb_ind_2];
            ap_ufixed< 5,-6> f_x_msb_2_h;
            f_x_msb_2_h(4,0) = f_x_msb_2(45,41);
            ap_ufixed< 9,-2> exp_x_msb_2_m_1_h = f_x_msb_2_h + x_msb_2;
            ap_ufixed<50,-2> exp_x_msb_2_m_1;
            exp_x_msb_2_m_1(49,41) = exp_x_msb_2_m_1_h(8,0);
            exp_x_msb_2_m_1(40, 0) = f_x_msb_2(40,0);
    
            ap_ufixed<98,-12> f_x_msb_2_3_4_lsb = exp_x_msb_2_m_1 * exp_x_msb_3_4_lsb_m_1;
            ap_ufixed<46,-12> f_x_msb_2_3_4_lsb_s = f_x_msb_2_3_4_lsb;
            ap_ufixed<50,-2> exp_x_msb_2_3_4_lsb_m_1 = f_x_msb_2_3_4_lsb_s + exp_x_msb_2_m_1 + exp_x_msb_3_4_lsb_m_1;
    
            const ap_ufixed<50,23> exp_x_msb_1_table[256] = {
"0x1.0000000p0", "0x1.2216045p0", "0x1.48B5E3Cp0", "0x1.747A513p0", 
"0x1.A61298Ep0", "0x1.DE455DFp0", "0x2.1DF3B68p0", "0x2.661CB0Fp0", 
"0x2.B7E1516p0", "0x3.1489176p0", "0x3.7D871DBp0", "0x3.F47FE87p0", 
"0x4.7B4FF99p0", "0x5.1413452p0", "0x5.C12DA41p0", "0x6.855466Ep0", 
"0x7.63992E3p0", "0x8.5F7635Bp0", "0x9.7CDC417p0", "0xA.C042667p0", 
"0xC.2EB7EC9p0", "0xD.CDF892Ep0", "0xF.A48385Ep0", "0x11.B9B5652p0", 
"0x14.15E5BF6p0", "0x16.C2887C1p0", "0x19.CA53B78p0", "0x1D.396AA97p0", 
"0x21.1D8E427p0", "0x25.865441Cp0", "0x2A.8565A14p0", "0x30.2EC550Bp0", 
"0x36.99205C4p0", "0x3D.DE28BF3p0", "0x46.1AFC4DBp0", "0x4F.7099532p0", 
"0x5A.0462B78p0", "0x66.00B5BC9p0", "0x73.9593ABBp0", "0x82.F9621ADp0", 
"0x94.69C4CB8p0", "0xA8.2C948C1p0", "0xBE.90F6F83p0", "0xD7.F09B78Cp0", 
"0xF4.B122790p0", "0x115.45B4704p0", "0x13A.30CF1CCp0", "0x164.0650296p0", 
"0x193.6DC5690p0", "0x1C9.250BEDCp0", "0x206.03487A3p0", "0x24A.FC4533Cp0", 
"0x299.2442102p0", "0x2F1.B447462p0", "0x356.0F0B0F9p0", "0x3C7.C67E5BEp0", 
"0x448.A216ABBp0", "0x4DA.A5EE46Ap0", "0x580.1AD754Fp0", "0x63B.9782341p0", 
"0x710.0ADBAC7p0", "0x800.C7CC8E3p0", "0x911.9289C39p0", "0xA46.AFAA2ADp0", 
"0xBA4.F53EA38p0", "0xD31.DE30C52p0", "0xEF3.A035D57p0", "0x10F1.44ADE60p0", 
"0x1332.C4D2B7Cp0", "0x15C1.29A744Cp0", "0x18A6.B027DA0p0", "0x1BEE.F24BB12p0", 
"0x1FA7.157C470p0", "0x23DD.FF3C8BAp0", "0x28A4.90D2CCEp0", "0x2E0D.EAE454Bp0", 
"0x342F.BA11823p0", "0x3B22.8DC5359p0", "0x4302.3A933EEp0", "0x4BEE.49AFCDCp0", 
"0x560A.773E541p0", "0x617F.4171BF9p0", "0x6E7A.8ABB4A8p0", "0x7D30.5191764p0", 
"0x8DDB.80AF426p0", "0xA0BE.DB0DB10p0", "0xB626.0748BAEp0", "0xCE66.BE9EAADp0", 
"0xE9E2.2447727p0", "0x10906.4A831F8p0", "0x12C4F.EB75B2Ap0", "0x1544C.5CB6133p0", 
"0x1819B.C560F61p0", "0x1B4F3.9F8AF64p0", "0x1EF21.8F1B001p0", "0x2310E.996C40Fp0", 
"0x27BC2.CA9A6F9p0", "0x2D069.571279Ap0", "0x33055.49F35D9p0", "0x39D06.D2FEF17p0", 
"0x41831.49596E9p0", "0x4A3C1.FB2AC39p0", "0x541E7.E56949Dp0", "0x5F51C.72B7532p0", 
"0x6C02D.645AB25p0", "0x7A648.0CFF534p0", "0x8AB06.0A3EE9Fp0", "0x9D27B.AFE4CF1p0", 
"0xB2148.5EAE56Cp0", "0xC9CA9.07F86EFp0", "0xE4A8D.2881EDBp0", "0x1031AE.8E4996Cp0", 
"0x1259AC.48BF05Dp0", "0x14CB29.2F2B31Fp0", "0x178FEE.7792E44p0", "0x1AB312.E89CD0Dp0", 
"0x1E4127.437732Bp0", "0x224868.979FC2Ep0", "0x26D8F9.4A204BEp0", "0x2C0521.B4A8E0Bp0", 
"0x31E199.5F5A550p0", "0x3885D9.FA89D00p0", "0x400C7D.64D3386p0", "0x4893A8.361032Dp0", 
"0x523D82.79EDAEFp0", "0x5D30C0.7DAB686p0", "0x69993D.D4F2D95p0", "0x77A8AD.02A7C71p0", 
"0x0.000001Ep0", "0x0.0000022p0", "0x0.0000026p0", "0x0.000002Bp0", 
"0x0.0000031p0", "0x0.0000038p0", "0x0.000003Fp0", "0x0.0000048p0", 
"0x0.0000052p0", "0x0.000005Dp0", "0x0.0000069p0", "0x0.0000077p0", 
"0x0.0000087p0", "0x0.0000099p0", "0x0.00000ADp0", "0x0.00000C4p0", 
"0x0.00000DFp0", "0x0.00000FCp0", "0x0.000011Ep0", "0x0.0000144p0", 
"0x0.0000170p0", "0x0.00001A1p0", "0x0.00001D8p0", "0x0.0000217p0", 
"0x0.000025Ep0", "0x0.00002AFp0", "0x0.000030Bp0", "0x0.0000372p0", 
"0x0.00003E8p0", "0x0.000046Dp0", "0x0.0000504p0", "0x0.00005AFp0", 
"0x0.0000671p0", "0x0.000074Cp0", "0x0.0000845p0", "0x0.000095Fp0", 
"0x0.0000A9Fp0", "0x0.0000C09p0", "0x0.0000DA3p0", "0x0.0000F74p0", 
"0x0.0001183p0", "0x0.00013D8p0", "0x0.000167Cp0", "0x0.000197Bp0", 
"0x0.0001CDFp0", "0x0.00020B7p0", "0x0.0002513p0", "0x0.0002A02p0", 
"0x0.0002F9Ap0", "0x0.00035F1p0", "0x0.0003D20p0", "0x0.0004543p0", 
"0x0.0004E7Cp0", "0x0.00058F0p0", "0x0.00064C7p0", "0x0.0007232p0", 
"0x0.0008167p0", "0x0.00092A2p0", "0x0.000A628p0", "0x0.000BC48p0", 
"0x0.000D55Ap0", "0x0.000F1C2p0", "0x0.00111F3p0", "0x0.001366Cp0", 
"0x0.0015FC2p0", "0x0.0018E98p0", "0x0.001C3AAp0", "0x0.001FFCEp0", 
"0x0.00243F3p0", "0x0.002912Bp0", "0x0.002E8ABp0", "0x0.0034BD2p0", 
"0x0.003BC2Dp0", "0x0.0043B7Dp0", "0x0.004CBC1p0", "0x0.0056F3Ap0", 
"0x0.0062878p0", "0x0.006FA5Fp0", "0x0.007E83Ap0", "0x0.008F5C0p0", 
"0x0.00A2728p0", "0x0.00B813Bp0", "0x0.00D0963p0", "0x0.00EC5C1p0", 
"0x0.010BD4Ap0", "0x0.012F7DEp0", "0x0.0157E6Bp0", "0x0.0185B0Fp0", 
"0x0.01B993Fp0", "0x0.01F45F9p0", "0x0.0236FF5p0", "0x0.02827E0p0", 
"0x0.02D80A0p0", "0x0.0338F9Fp0", "0x0.03A6D22p0", "0x0.04234A7p0", 
"0x0.04B0556p0", "0x0.0550280p0", "0x0.0605424p0", "0x0.06D279Ap0", 
"0x0.07BB040p0", "0x0.08C284Cp0", "0x0.09ED1B4p0", "0x0.0B3F736p0", 
"0x0.0CBED86p0", "0x0.0E7149Cp0", "0x0.105D938p0", "0x0.128B697p0", 
"0x0.150385Cp0", "0x0.17CFCC2p0", "0x0.1AFB718p0", "0x0.1E9328Bp0", 
"0x0.22A5554p0", "0x0.2742456p0", "0x0.2C7C72Fp0", "0x0.3268CDBp0", 
"0x0.391F0EEp0", "0x0.40BA188p0", "0x0.495860Dp0", "0x0.531C6C9p0", 
"0x0.5E2D58Dp0", "0x0.6AB7782p0", "0x0.78ED03Ap0", "0x0.8906E49p0", 
"0x0.9B4597Ep0", "0x0.AFF230Ap0", "0x0.C75F7CFp0", "0x0.E1EB512p0", 
            };
            ap_ufixed<50,23> exp_x_msb_1 = exp_x_msb_1_table[x_msb_ind_1];
            ap_ufixed<100,21> y_lo = exp_x_msb_1 * exp_x_msb_2_3_4_lsb_m_1;
            ap_ufixed<48,21> y_lo_s = y_lo;
            ap_ufixed<50,23> y_l = y_lo_s + exp_x_msb_1;
            y = y_l;
        }
        if (I_<24) { // overflow
            bool overf = 0;
            #pragma unroll
            for (int j = 45; j >= I_+22; j--) {
                if (y[j])
                    overf = 1;
            }
            if (overf) {
                #pragma unroll
                for (int j = 45; j >= I_+22; j--)
                    y[j] = 0;
                #pragma unroll
                for (int j = I_+21; j >= 0; j--)
                    y[j] = 1;
            }
        }
        r = y;
    } else { // I_s_ = 6, M_ = 24..32, I_ <= 33, F_ <= 32, W_ <= 65
        //ap_fixed<65,33> x_l = x;
        ap_fixed<65,33> x_l = 0;
        #pragma unroll
        for (int j = 32-F_; j < 32+I_; j++)
            x_l[j] = x[j-(32-F_)];
        #pragma unroll
        for (int j = 32+I_; j <= 64; j++)
            x_l[j] = x[W_-1];
        ap_ufixed<64,32> y = 0;
        ap_fixed<6,6> x_l_int;
        x_l_int(5,0) = x_l(37,32);
        ap_ufixed<32,0> x_l_fract;
        x_l_fract(31,0) = x_l(31,0);
        bool overf = 0;
        #pragma unroll
        for (int j = 37; j < 64; j++)
            if (x_l[64]!=x_l[j])
                overf = 1;
        //if (overf||(x_l>="0x16.2e42fefap0")) { // overflow
        if (overf||(x_l_int>22)||((x_l_int==22)&&(x_l_fract>="0x0.2e42fefap0"))) { // overflow
            if (!x_l[64]) {
                #pragma unroll
                for (int j = 0; j < 64; j++) y[j] = 1;
            }
        } else {
            ap_uint<8> x_msb_ind_1;
            x_msb_ind_1[7] = x_l[64];
            x_msb_ind_1(6,0) = x_l(36,30);
            ap_uint<8> x_msb_ind_2;
            x_msb_ind_2(7,0) = x_l(29,22);
            ap_uint<8> x_msb_ind_3;
            x_msb_ind_3(7,0) = x_l(21,14);
            ap_uint<8> x_msb_ind_4;
            x_msb_ind_4(7,0) = x_l(13,6);
            ap_uint<6> x_lsb_ind;
            x_lsb_ind(5,0) = x_l(5,0);

            ap_ufixed<8, -2> x_msb_2 = x_l;
            ap_ufixed<8,-10> x_msb_3 = x_l;
            ap_ufixed<8,-18> x_msb_4 = x_l;
            ap_ufixed<6,-26> x_lsb   = x_l;

            const ap_ufixed<31,-37> f_x_msb_4_table[256] = {
"0x0.000000000000000000p0", "0x0.000000000000080000p0", "0x0.000000000000200000p0", "0x0.000000000000480000p0", 
"0x0.000000000000800000p0", "0x0.000000000000C80000p0", "0x0.000000000001200000p0", "0x0.000000000001880000p0", 
"0x0.000000000002000001p0", "0x0.000000000002880001p0", "0x0.000000000003200002p0", "0x0.000000000003C80003p0", 
"0x0.000000000004800004p0", "0x0.000000000005480005p0", "0x0.000000000006200007p0", "0x0.000000000007080008p0", 
"0x0.00000000000800000Ap0", "0x0.00000000000908000Cp0", "0x0.00000000000A20000Fp0", "0x0.00000000000B480011p0", 
"0x0.00000000000C800014p0", "0x0.00000000000DC80018p0", "0x0.00000000000F20001Bp0", "0x0.00000000001088001Fp0", 
"0x0.000000000012000024p0", "0x0.000000000013880028p0", "0x0.00000000001520002Dp0", "0x0.000000000016C80033p0", 
"0x0.000000000018800039p0", "0x0.00000000001A48003Fp0", "0x0.00000000001C200046p0", "0x0.00000000001E08004Dp0", 
"0x0.000000000020000055p0", "0x0.00000000002208005Dp0", "0x0.000000000024200066p0", "0x0.00000000002648006Fp0", 
"0x0.000000000028800079p0", "0x0.00000000002AC80083p0", "0x0.00000000002D20008Ep0", "0x0.00000000002F88009Ap0", 
"0x0.0000000000320000A6p0", "0x0.0000000000348800B3p0", "0x0.0000000000372000C0p0", "0x0.000000000039C800CFp0", 
"0x0.00000000003C8000DDp0", "0x0.00000000003F4800EDp0", "0x0.0000000000422000FDp0", "0x0.00000000004508010Ep0", 
"0x0.000000000048000120p0", "0x0.00000000004B080132p0", "0x0.00000000004E200145p0", "0x0.000000000051480159p0", 
"0x0.00000000005480016Ep0", "0x0.000000000057C80183p0", "0x0.00000000005B20019Ap0", "0x0.00000000005E8801B1p0", 
"0x0.0000000000620001C9p0", "0x0.0000000000658801E2p0", "0x0.0000000000692001FCp0", "0x0.00000000006CC80216p0", 
"0x0.000000000070800232p0", "0x0.00000000007448024Fp0", "0x0.00000000007820026Cp0", "0x0.00000000007C08028Bp0", 
"0x0.0000000000800002AAp0", "0x0.0000000000840802CBp0", "0x0.0000000000882002ECp0", "0x0.00000000008C48030Fp0", 
"0x0.000000000090800332p0", "0x0.000000000094C80357p0", "0x0.00000000009920037Dp0", "0x0.00000000009D8803A4p0", 
"0x0.0000000000A20003CCp0", "0x0.0000000000A68803F5p0", "0x0.0000000000AB20041Fp0", "0x0.0000000000AFC8044Ap0", 
"0x0.0000000000B4800477p0", "0x0.0000000000B94804A4p0", "0x0.0000000000BE2004D3p0", "0x0.0000000000C3080503p0", 
"0x0.0000000000C8000535p0", "0x0.0000000000CD080567p0", "0x0.0000000000D220059Bp0", "0x0.0000000000D74805D1p0", 
"0x0.0000000000DC800607p0", "0x0.0000000000E1C8063Fp0", "0x0.0000000000E7200678p0", "0x0.0000000000EC8806B2p0", 
"0x0.0000000000F20006EEp0", "0x0.0000000000F788072Bp0", "0x0.0000000000FD20076Ap0", "0x0.000000000102C807AAp0", 
"0x0.0000000001088007EBp0", "0x0.00000000010E48082Ep0", "0x0.000000000114200872p0", "0x0.00000000011A0808B8p0", 
"0x0.000000000120000900p0", "0x0.000000000126080948p0", "0x0.00000000012C200993p0", "0x0.0000000001324809DEp0", 
"0x0.000000000138800A2Cp0", "0x0.00000000013EC80A7Bp0", "0x0.000000000145200ACBp0", "0x0.00000000014B880B1Dp0", 
"0x0.000000000152000B71p0", "0x0.000000000158880BC6p0", "0x0.00000000015F200C1Dp0", "0x0.000000000165C80C76p0", 
"0x0.00000000016C800CD0p0", "0x0.000000000173480D2Cp0", "0x0.00000000017A200D8Ap0", "0x0.000000000181080DE9p0", 
"0x0.000000000188000E4Ap0", "0x0.00000000018F080EADp0", "0x0.000000000196200F12p0", "0x0.00000000019D480F78p0", 
"0x0.0000000001A4800FE0p0", "0x0.0000000001ABC8104Ap0", "0x0.0000000001B32010B6p0", "0x0.0000000001BA881124p0", 
"0x0.0000000001C2001194p0", "0x0.0000000001C9881205p0", "0x0.0000000001D1201278p0", "0x0.0000000001D8C812EEp0", 
"0x0.0000000001E0801365p0", "0x0.0000000001E84813DEp0", "0x0.0000000001F0201459p0", "0x0.0000000001F80814D6p0", 
"0x0.000000000200001555p0", "0x0.0000000002080815D6p0", "0x0.000000000210201659p0", "0x0.0000000002184816DEp0", 
"0x0.000000000220801765p0", "0x0.000000000228C817EEp0", "0x0.000000000231201879p0", "0x0.000000000239881907p0", 
"0x0.000000000242001996p0", "0x0.00000000024A881A28p0", "0x0.000000000253201ABBp0", "0x0.00000000025BC81B51p0", 
"0x0.000000000264801BE9p0", "0x0.00000000026D481C84p0", "0x0.000000000276201D20p0", "0x0.00000000027F081DBFp0", 
"0x0.000000000288001E60p0", "0x0.000000000291081F03p0", "0x0.00000000029A201FA8p0", "0x0.0000000002A3482050p0", 
"0x0.0000000002AC8020FAp0", "0x0.0000000002B5C821A6p0", "0x0.0000000002BF202255p0", "0x0.0000000002C8882306p0", 
"0x0.0000000002D20023B9p0", "0x0.0000000002DB88246Fp0", "0x0.0000000002E5202527p0", "0x0.0000000002EEC825E1p0", 
"0x0.0000000002F880269Ep0", "0x0.00000000030248275Dp0", "0x0.00000000030C20281Fp0", "0x0.0000000003160828E3p0", 
"0x0.0000000003200029AAp0", "0x0.00000000032A082A73p0", "0x0.000000000334202B3Fp0", "0x0.00000000033E482C0Dp0", 
"0x0.000000000348802CDEp0", "0x0.000000000352C82DB2p0", "0x0.00000000035D202E88p0", "0x0.000000000367882F60p0", 
"0x0.00000000037200303Cp0", "0x0.00000000037C883119p0", "0x0.0000000003872031FAp0", "0x0.000000000391C832DDp0", 
"0x0.00000000039C8033C3p0", "0x0.0000000003A74834ABp0", "0x0.0000000003B2203596p0", "0x0.0000000003BD083684p0", 
"0x0.0000000003C8003775p0", "0x0.0000000003D3083868p0", "0x0.0000000003DE20395Ep0", "0x0.0000000003E9483A57p0", 
"0x0.0000000003F4803B53p0", "0x0.0000000003FFC83C52p0", "0x0.00000000040B203D53p0", "0x0.000000000416883E57p0", 
"0x0.000000000422003F5Ep0", "0x0.00000000042D884068p0", "0x0.000000000439204175p0", "0x0.000000000444C84285p0", 
"0x0.000000000450804397p0", "0x0.00000000045C4844ADp0", "0x0.0000000004682045C5p0", "0x0.0000000004740846E1p0", 
"0x0.000000000480004800p0", "0x0.00000000048C084921p0", "0x0.000000000498204A46p0", "0x0.0000000004A4484B6Dp0", 
"0x0.0000000004B0804C98p0", "0x0.0000000004BCC84DC5p0", "0x0.0000000004C9204EF6p0", "0x0.0000000004D588502Ap0", 
"0x0.0000000004E2005161p0", "0x0.0000000004EE88529Bp0", "0x0.0000000004FB2053D8p0", "0x0.000000000507C85518p0", 
"0x0.00000000051480565Cp0", "0x0.0000000005214857A3p0", "0x0.00000000052E2058EDp0", "0x0.00000000053B085A3Ap0", 
"0x0.000000000548005B8Ap0", "0x0.000000000555085CDEp0", "0x0.000000000562205E35p0", "0x0.00000000056F485F8Fp0", 
"0x0.00000000057C8060ECp0", "0x0.000000000589C8624Dp0", "0x0.0000000005972063B1p0", "0x0.0000000005A4886519p0", 
"0x0.0000000005B2006684p0", "0x0.0000000005BF8867F2p0", "0x0.0000000005CD206963p0", "0x0.0000000005DAC86AD8p0", 
"0x0.0000000005E8806C51p0", "0x0.0000000005F6486DCDp0", "0x0.000000000604206F4Cp0", "0x0.0000000006120870CFp0", 
"0x0.000000000620007255p0", "0x0.00000000062E0873DFp0", "0x0.00000000063C20756Cp0", "0x0.00000000064A4876FDp0", 
"0x0.000000000658807891p0", "0x0.000000000666C87A29p0", "0x0.000000000675207BC4p0", "0x0.000000000683887D64p0", 
"0x0.000000000692007F06p0", "0x0.0000000006A08880ADp0", "0x0.0000000006AF208256p0", "0x0.0000000006BDC88404p0", 
"0x0.0000000006CC8085B5p0", "0x0.0000000006DB48876Ap0", "0x0.0000000006EA208923p0", "0x0.0000000006F9088ADFp0", 
"0x0.000000000708008CA0p0", "0x0.000000000717088E63p0", "0x0.00000000072620902Bp0", "0x0.0000000007354891F6p0", 
"0x0.0000000007448093C6p0", "0x0.000000000753C89599p0", "0x0.000000000763209770p0", "0x0.00000000077288994Ap0", 
"0x0.000000000782009B29p0", "0x0.000000000791889D0Bp0", "0x0.0000000007A1209EF2p0", "0x0.0000000007B0C8A0DCp0", 
"0x0.0000000007C080A2CAp0", "0x0.0000000007D048A4BCp0", "0x0.0000000007E020A6B2p0", "0x0.0000000007F008A8ACp0", 
            };
            ap_ufixed<31,-37> f_x_msb_4 = f_x_msb_4_table[x_msb_ind_4];

            const ap_ufixed<12,-53> f_x_lsb_table[64] = {
"0x0.00000000000000000p0", "0x0.00000000000000008p0", "0x0.00000000000000020p0", "0x0.00000000000000048p0",
"0x0.00000000000000080p0", "0x0.000000000000000c8p0", "0x0.00000000000000120p0", "0x0.00000000000000188p0",
"0x0.00000000000000200p0", "0x0.00000000000000288p0", "0x0.00000000000000320p0", "0x0.000000000000003c8p0",
"0x0.00000000000000480p0", "0x0.00000000000000548p0", "0x0.00000000000000620p0", "0x0.00000000000000708p0",
"0x0.00000000000000800p0", "0x0.00000000000000908p0", "0x0.00000000000000a20p0", "0x0.00000000000000b48p0",
"0x0.00000000000000c80p0", "0x0.00000000000000dc8p0", "0x0.00000000000000f20p0", "0x0.00000000000001088p0",
"0x0.00000000000001200p0", "0x0.00000000000001388p0", "0x0.00000000000001520p0", "0x0.000000000000016c8p0",
"0x0.00000000000001880p0", "0x0.00000000000001a48p0", "0x0.00000000000001c20p0", "0x0.00000000000001e08p0",
"0x0.00000000000002000p0", "0x0.00000000000002208p0", "0x0.00000000000002420p0", "0x0.00000000000002648p0",
"0x0.00000000000002880p0", "0x0.00000000000002ac8p0", "0x0.00000000000002d20p0", "0x0.00000000000002f88p0",
"0x0.00000000000003200p0", "0x0.00000000000003488p0", "0x0.00000000000003720p0", "0x0.000000000000039c8p0",
"0x0.00000000000003c80p0", "0x0.00000000000003f48p0", "0x0.00000000000004220p0", "0x0.00000000000004508p0",
"0x0.00000000000004800p0", "0x0.00000000000004b08p0", "0x0.00000000000004e20p0", "0x0.00000000000005148p0",
"0x0.00000000000005480p0", "0x0.000000000000057c8p0", "0x0.00000000000005b20p0", "0x0.00000000000005e88p0",
"0x0.00000000000006200p0", "0x0.00000000000006588p0", "0x0.00000000000006920p0", "0x0.00000000000006cc8p0",
"0x0.00000000000007080p0", "0x0.00000000000007448p0", "0x0.00000000000007820p0", "0x0.00000000000007c08p0",
            };
            ap_ufixed<12,-53> f_x_lsb = f_x_lsb_table[x_lsb_ind];

            ap_ufixed<14,-44> f_x_msb_4_lsb = x_msb_4 * x_lsb;

            ap_ufixed<15,-43> exp_x_msb_4_lsb_m_1_m1 = 0;
            exp_x_msb_4_lsb_m_1_m1(4,0) = f_x_lsb(11,7);
            exp_x_msb_4_lsb_m_1_m1 = exp_x_msb_4_lsb_m_1_m1 + f_x_msb_4_lsb;

            ap_ufixed<22,-43> exp_x_msb_4_lsb_m_1_m2 = exp_x_msb_4_lsb_m_1_m1;
            exp_x_msb_4_lsb_m_1_m2(6,0) = f_x_lsb(6,0);

            ap_ufixed<29,-36> exp_x_msb_4_lsb_m_1_m3 = 0;
            exp_x_msb_4_lsb_m_1_m3(27,0) = f_x_msb_4(30,3);
            exp_x_msb_4_lsb_m_1_m3 = exp_x_msb_4_lsb_m_1_m3 + exp_x_msb_4_lsb_m_1_m2;

            ap_ufixed<32,-36> exp_x_msb_4_lsb_m_1_m = exp_x_msb_4_lsb_m_1_m3;
            exp_x_msb_4_lsb_m_1_m(2,0) = f_x_msb_4(2,0);

            ap_ufixed<50,-18> exp_x_msb_4_lsb_m_1 = 0;
            exp_x_msb_4_lsb_m_1(49,36) = x_l(13,0);
            exp_x_msb_4_lsb_m_1(31, 0) = exp_x_msb_4_lsb_m_1_m(31,0);


            const ap_ufixed<51,-21> f_x_msb_3_table[256] = {
"0x0", "0x0.00000000080000AAAAp0", "0x0.000000002000055556p0", "0x0.000000004800120003p0", 
"0x0.0000000080002AAAB5p0", "0x0.00000000C80053556Fp0", "0x0.000000012000900036p0", "0x0.000000018800E4AB0Ep0", 
"0x0.000000020001555600p0", "0x0.000000028801E60111p0", "0x0.0000000320029AAC4Bp0", "0x0.00000003C8037757B7p0", 
"0x0.000000048004800360p0", "0x0.000000054805B8AF50p0", "0x0.000000062007255B96p0", "0x0.000000070808CA083Dp0", 
"0x0.00000008000AAAB555p0", "0x0.00000009080CCB62EDp0", "0x0.0000000A200F301116p0", "0x0.0000000B4811DCBFE0p0", 
"0x0.0000000C8014D56F60p0", "0x0.0000000DC8181E1FA7p0", "0x0.0000000F201BBAD0CBp0", "0x0.00000010881FAF82E1p0", 
"0x0.000000120024003600p0", "0x0.000000138828B0EA3Fp0", "0x0.00000015202DC59FB6p0", "0x0.00000016C83342567Fp0", 
"0x0.0000001880392B0EB5p0", "0x0.0000001A483F83C874p0", "0x0.0000001C20465083D6p0", "0x0.0000001E084D9540FBp0", 
"0x0.000000200055560001p0", "0x0.00000022085D96C106p0", "0x0.0000002420665B842Cp0", "0x0.00000026486FA84995p0", 
"0x0.000000288079811161p0", "0x0.0000002AC883E9DBB6p0", "0x0.0000002D208EE6A8B8p0", "0x0.0000002F889A7B788Cp0", 
"0x0.0000003200A6AC4B58p0", "0x0.0000003488B37D2145p0", "0x0.0000003720C0F1FA7Ap0", "0x0.00000039C8CF0ED721p0", 
"0x0.0000003C80DDD7B765p0", "0x0.0000003F48ED509B71p0", "0x0.0000004220FD7D8371p0", "0x0.00000045090E626F94p0", 
"0x0.000000480120036008p0", "0x0.0000004B09326454FBp0", "0x0.0000004E2145894E9Fp0", "0x0.000000514959764D26p0", 
"0x0.00000054816E2F50C1p0", "0x0.00000057C983B859A4p0", "0x0.0000005B219A156804p0", "0x0.0000005E89B14A7C16p0", 
"0x0.0000006201C95B9611p0", "0x0.0000006589E24CB62Cp0", "0x0.0000006921FC21DCA0p0", "0x0.0000006CCA16DF09A6p0", 
"0x0.000000708232883D78p0", "0x0.000000744A4F217853p0", "0x0.00000078226CAEBA73p0", "0x0.0000007C0A8B340414p0", 
"0x0.0000008002AAB55577p0", "0x0.000000840ACB36AEDAp0", "0x0.0000008822ECBC107Dp0", "0x0.0000008C4B0F497AA3p0", 
"0x0.000000908332E2ED8Ep0", "0x0.00000094CB578C6981p0", "0x0.00000099237D49EEC0p0", "0x0.0000009D8BA41F7D92p0", 
"0x0.000000A203CC11163Dp0", "0x0.000000A68BF522B908p0", "0x0.000000AB241F58663Cp0", "0x0.000000AFCC4AB61E22p0", 
"0x0.000000B484773FE105p0", "0x0.000000B94CA4F9AF31p0", "0x0.000000BE24D3E788F1p0", "0x0.000000C30D040D6E94p0", 
"0x0.000000C805356F6068p0", "0x0.000000CD0D68115EBCp0", "0x0.000000D2259BF769E1p0", "0x0.000000D74DD1258228p0", 
"0x0.000000DC86079FA7E4p0", "0x0.000000E1CE3F69DB69p0", "0x0.000000E72678881D0Bp0", "0x0.000000EC8EB2FE6D1Fp0", 
"0x0.000000F206EED0CBFDp0", "0x0.000000F78F2C0339FAp0", "0x0.000000FD276A99B771p0", "0x0.00000102CFAA9844BBp0", 
"0x0.0000010887EC02E231p0", "0x0.0000010E502EDD9030p0", "0x0.0000011428732C4F14p0", "0x0.0000011A10B8F31F3Bp0", 
"0x0.000001200900360103p0", "0x0.000001261148F8F4CBp0", "0x0.0000012C29933FFAF5p0", "0x0.0000013251DF0F13E1p0", 
"0x0.000001388A2C6A3FF3p0", "0x0.0000013ED27B557F8Dp0", "0x0.000001452ACBD4D315p0", "0x0.0000014B931DEC3AEFp0", 
"0x0.000001520B719FB782p0", "0x0.0000015893C6F34937p0", "0x0.0000015F2C1DEAF074p0", "0x0.00000165D4768AADA5p0", 
"0x0.0000016C8CD0D68133p0", "0x0.00000173552CD26B89p0", "0x0.0000017A2D8A826D16p0", "0x0.0000018115E9EA8645p0", 
"0x0.000001880E4B0EB785p0", "0x0.0000018F16ADF30147p0", "0x0.000001962F129B63FAp0", "0x0.0000019D57790BE010p0", 
"0x0.000001A48FE14875FBp0", "0x0.000001ABD84B552630p0", "0x0.000001B330B735F122p0", "0x0.000001BA9924EED748p0", 
"0x0.000001C2119483D917p0", "0x0.000001C99A05F8F707p0", "0x0.000001D13279523191p0", "0x0.000001D8DAEE93892Ep0", 
"0x0.000001E09365C0FE59p0", "0x0.000001E85BDEDE918Dp0", "0x0.000001F03459F04347p0", "0x0.000001F81CD6FA1405p0", 
"0x0.000002001556000444p0", "0x0.000002081DD7061485p0", "0x0.00000210365A104547p0", "0x0.000002185EDF22970Dp0", 
"0x0.000002209766410A5Ap0", "0x0.00000228DFEF6F9FAFp0", "0x0.00000231387AB25793p0", "0x0.00000239A1080D328Ap0", 
"0x0.00000242199784311Cp0", "0x0.0000024AA2291B53CFp0", "0x0.000002533ABCD69B2Dp0", "0x0.0000025BE352BA07BEp0", 
"0x0.000002649BEAC99A0Dp0", "0x0.0000026D64850952A7p0", "0x0.000002763D217D3216p0", "0x0.0000027F25C02938EAp0", 
"0x0.000002881E611167B0p0", "0x0.00000291270439BEF8p0", "0x0.0000029A3FA9A63F53p0", "0x0.000002A368515AE951p0", 
"0x0.000002ACA0FB5BBD86p0", "0x0.000002B5E9A7ACBC86p0", "0x0.000002BF425651E6E4p0", "0x0.000002C8AB074F3D36p0", 
"0x0.000002D223BAA8C013p0", "0x0.000002DBAC70627012p0", "0x0.000002E54528804DCDp0", "0x0.000002EEEDE30659DBp0", 
"0x0.000002F8A69FF894D9p0", "0x0.000003026F5F5AFF61p0", "0x0.0000030C4821319A10p0", "0x0.0000031630E5806584p0", 
"0x0.0000032029AC4B625Bp0", "0x0.0000032A3275969134p0", "0x0.000003344B4165F2B1p0", "0x0.0000033E740FBD8772p0", 
"0x0.00000348ACE0A1501Bp0", "0x0.00000352F5B4154D4Fp0", "0x0.0000035D4E8A1D7FB2p0", "0x0.00000367B762BDE7EAp0", 
"0x0.00000372303DFA869Ep0", "0x0.0000037CB91BD75C75p0", "0x0.0000038751FC586A18p0", "0x0.00000391FADF81B02Fp0", 
"0x0.0000039CB3C5572F67p0", "0x0.000003A77CADDCE86Ap0", "0x0.000003B2559916DBE4p0", "0x0.000003BD3E87090A84p0", 
"0x0.000003C83777B774F8p0", "0x0.000003D3406B261BF0p0", "0x0.000003DE596159001Cp0", "0x0.000003E9825A54222Dp0", 
"0x0.000003F4BB561B82D7p0", "0x0.000004000454B322CCp0", "0x0.0000040B5D561F02C2p0", "0x0.00000416C65A63236Ep0", 
"0x0.000004223F61838586p0", "0x0.0000042DC86B8429C2p0", "0x0.0000043961786910DBp0", "0x0.000004450A88363B8Ap0", 
"0x0.00000450C39AEFAA8Ap0", "0x0.0000045C8CB0995E96p0", "0x0.0000046865C937586Bp0", "0x0.000004744EE4CD98C6p0", 
"0x0.000004804803602067p0", "0x0.0000048C5124F2F00Cp0", "0x0.000004986A498A0876p0", "0x0.000004A49371296A67p0", 
"0x0.000004B0CC9BD516A1p0", "0x0.000004BD15C9910DE8p0", "0x0.000004C96EFA615101p0", "0x0.000004D5D82E49E0B0p0", 
"0x0.000004E251654EBDBDp0", "0x0.000004EEDA9F73E8F0p0", "0x0.000004FB73DCBD6310p0", "0x0.000005081D1D2F2CE7p0", 
"0x0.00000514D660CD4740p0", "0x0.000005219FA79BB2E7p0", "0x0.0000052E78F19E70A8p0", "0x0.0000053B623ED98150p0", 
"0x0.000005485B8F50E5AFp0", "0x0.0000055564E3089E93p0", "0x0.000005627E3A04ACCEp0", "0x0.0000056FA794491131p0", 
"0x0.0000057CE0F1D9CC8Ep0", "0x0.0000058A2A52BADFBAp0", "0x0.0000059783B6F04B88p0", "0x0.000005A4ED1E7E10CFp0", 
"0x0.000005B26689683064p0", "0x0.000005BFEFF7B2AB20p0", "0x0.000005CD89696181DBp0", "0x0.000005DB32DE78B56Fp0", 
"0x0.000005E8EC56FC46B6p0", "0x0.000005F6B5D2F0368Cp0", "0x0.000006048F525885CDp0", "0x0.0000061278D5393558p0", 
"0x0.00000620725B96460Ap0", "0x0.0000062E7BE573B8C3p0", "0x0.0000063C9572D58E64p0", "0x0.0000064ABF03BFC7CEp0", 
"0x0.00000658F8983665E5p0", "0x0.0000066742303D698Bp0", "0x0.000006759BCBD8D3A5p0", "0x0.00000684056B0CA519p0", 
"0x0.000006927F0DDCDECEp0", "0x0.000006A108B44D81AAp0", "0x0.000006AFA25E628E98p0", "0x0.000006BE4C0C20067Fp0", 
"0x0.000006CD05BD89EA4Bp0", "0x0.000006DBCF72A43AE8p0", "0x0.000006EAA92B72F942p0", "0x0.000006F992E7FA2646p0", 
"0x0.000007088CA83DC2E4p0", "0x0.00000717966C41D00Ap0", "0x0.00000726B0340A4EAAp0", "0x0.00000735D9FF9B3FB6p0", 
"0x0.0000074513CEF8A41Ep0", "0x0.000007545DA2267CD9p0", "0x0.00000763B77928CAD9p0", "0x0.000007732154038F14p0", 
"0x0.000007829B32BACA82p0", "0x0.000007922515527E1Ap0", "0x0.000007A1BEFBCEAAD4p0", "0x0.000007B168E63351AAp0", 
"0x0.000007C122D4847397p0", "0x0.000007D0ECC6C61195p0", "0x0.000007E0C6BCFC2CA3p0", "0x0.000007F0B0B72AC5BEp0", 
            };
            ap_ufixed<51,-21> f_x_msb_3 = f_x_msb_3_table[x_msb_ind_3];
            ap_ufixed<62,-10> exp_x_msb_3_m_1 = 0;
            exp_x_msb_3_m_1(61,54) = x_msb_3(7,0);
            exp_x_msb_3_m_1(50, 0) = f_x_msb_3(50,0);

            ap_ufixed<50,-10> exp_x_msb_3_m_1_s = exp_x_msb_3_m_1;
            ap_ufixed<100,-28> f_x_msb_3_4_lsb = exp_x_msb_3_m_1_s * exp_x_msb_4_lsb_m_1;
            ap_ufixed<44,-28> f_x_msb_3_4_lsb_s = f_x_msb_3_4_lsb;
            ap_ufixed<63, -9> exp_x_msb_3_4_lsb_m_1 = f_x_msb_3_4_lsb_s + exp_x_msb_3_m_1 + exp_x_msb_4_lsb_m_1;


            const ap_ufixed<64,-4> f_x_msb_2_table[256] = {
"0x0", "0x0.00000800AAB555DDEp0", "0x0.00002005560011127p0", "0x0.00004812036081A9Cp0", 
"0x0.0000802AB55777D28p0", "0x0.0000C8536F6684062p0", "0x0.0001209036103740Dp0", "0x0.000188E50ED8634A0p0", 
"0x0.0002015600445B0C3p0", "0x0.000289E711DB32FD7p0", "0x0.0003229C4C260197Ep0", "0x0.0003CB79B8B01FE26p0", 
"0x0.0004848362076A08Dp0", "0x0.00054DBD53BC80058p0", "0x0.0006272B9A630659Dp0", "0x0.000710D24391E6D7Ap0", 
"0x0.00080AB55DE3917ABp0", "0x0.000914D8F8F63D524p0", "0x0.000A2F41256C297AFp0", "0x0.000B59F1F4EBDE291p0", 
"0x0.000C94EF7A206DC2Dp0", "0x0.000DE03DC8B9B60B1p0", "0x0.000F3BE0F56CA15C4p0", "0x0.0010A7DD15F367F40p0", 
"0x0.00122436410DD14E5p0", "0x0.0013B0F08E817591Ep0", "0x0.00154E101719FF0C5p0", "0x0.0016FB98F4A96BBEFp0", 
"0x0.0018B98F42084EFBDp0", "0x0.001A87F71B1613137p0", "0x0.001C66D49CB93B127p0", "0x0.001E562BE4DFA4904p0", 
"0x0.00205601127EC98E0p0", "0x0.00226658459402659p0", "0x0.002487359F24C7C99p0", "0x0.0026B89D413EF4D5Ep0", 
"0x0.0028FA934EF909304p0", "0x0.002B4D1BEC726B39Cp0", "0x0.002DB03B3ED3AA50Ep0", "0x0.003023F56C4EC123Fp0", 
"0x0.0032A84E9C1F58145p0", "0x0.00353D4AF68B07AA1p0", "0x0.0037E2EEA4E19B185p0", "0x0.003A993DD17D52D25p0", 
"0x0.003D603CA7C32730Fp0", "0x0.004037EF54230B293p0", "0x0.0043205A04182F12Fp0", "0x0.00461980E62943810p0", 
"0x0.0049236829E8BC292p0", "0x0.004C3E13FFF512DD8p0", "0x0.004F698899F90A966p0", "0x0.0052A5CA2AABF28D0p0", 
"0x0.0055F2DCE5D1E966Ep0", "0x0.005950C5003C20723p0", "0x0.005CBF86AFC91EF2Bp0", "0x0.00603F262B65057FCp0", 
"0x0.0063CFA7AB09D1732p0", "0x0.0067710F67BFA0687p0", "0x0.006B23619B9CF3CDEp0", "0x0.006EE6A281C6F4857p0", 
"0x0.0072BAD65671B6977p0", "0x0.0076A00156E07CF57p0", "0x0.007A9627C165FD4EDp0", "0x0.007E9D4DD564A3F5Ep0", 
"0x0.0082B577D34ED7D5Bp0", "0x0.0086DEA9FCA73E799p0", "0x0.008B18E8940100253p0", "0x0.008F6437DD000BFDAp0", 
"0x0.0093C09C1C595C43Ep0", "0x0.00982E1997D33A9FCp0", "0x0.009CACB49645847CCp0", "0x0.00A13C715F99EF773p0", 
"0x0.00A5DD543CCC4DDAFp0", "0x0.00AA8F6177EAD3336p0", "0x0.00AF529D5C1658EBCp0", "0x0.00B4270C3582A301Dp0", 
"0x0.00B90CB25176A4C8Bp0", "0x0.00BE0393FE4CC5BD7p0", "0x0.00C30BB58B73266CAp0", "0x0.00C8251B496BE5696p0", 
"0x0.00CD4FC989CD64555p0", "0x0.00D28BC49F428CFA3p0", "0x0.00D7D910DD8B16743p0", "0x0.00DD37B2997BCA6E6p0", 
"0x0.00E2A7AE28FECA6FBp0", "0x0.00E82907E313D5399p0", "0x0.00EDBBC41FD08C383p0", "0x0.00F35FE73860B9038p0", 
"0x0.00F91575870692F24p0", "0x0.00FEDC73671B04BDFp0", "0x0.0104B4E5350DF2386p0", "0x0.010A9ECF4E667E12Dp0", 
"0x0.01109A3611C34FB64p0", "0x0.0116A71DDEDAD92D8p0", "0x0.011CC58B167B9D206p0", "0x0.0122F5821A8C74E0Dp0", 
"0x0.012937074E0CD6893p0", "0x0.012F8A1F15151B2C6p0", "0x0.0135EECDD4D6C5172p0", "0x0.013C6517F39CC6233p0", 
"0x0.0142ED01D8CBC61C4p0", "0x0.0149868FECE26935Dp0", "0x0.015031C6997996937p0", "0x0.0156EEAA4944BEE23p0", 
"0x0.015DBD3F68122303Ep0", "0x0.01649D8A62CB1ACBEp0", "0x0.016B8F8FA7745BCDBp0", "0x0.01729353A52E403D8p0", 
"0x0.0179A8DACC350DE1Dp0", "0x0.0180D0298DE13D179p0", "0x0.018809445CA7BFE78p0", "0x0.018F542FAC1A492D6p0", 
"0x0.0196B0EFF0E793D15p0", "0x0.019E1F89A0DBAA128p0", "0x0.01A5A00132E02CE42p0", "0x0.01AD325B1EFC9B5BCp0", 
"0x0.01B4D69BDE569A322p0", "0x0.01BC8CC7EB323B553p0", "0x0.01C454E3C0F2458C8p0", "0x0.01CC2EF3DC187C2F6p0", 
"0x0.01D41AFCBA45E6ED2p0", "0x0.01DC1902DA3B19A6Dp0", "0x0.01E4290ABBD87C5BCp0", "0x0.01EC4B18E01E9326Fp0", 
"0x0.01F47F31C92E464FCp0", "0x0.01FCC559FA492A6B6p0", "0x0.02051D95F7D1C8917p0", "0x0.020D87EA474BE6A1Cp0", 
"0x0.0216045B6F5CCF9CEp0", "0x0.021E92EDF7CB9C0E4p0", "0x0.022733A669817A88Ap0", "0x0.022FE6894E89F834Cp0", 
"0x0.0238AB9B321349722p0", "0x0.024182E0A06E9289Bp0", "0x0.024A6C5E271030733p0", "0x0.02536818549001AC1p0", 
"0x0.025C7613B8A9AF215p0", "0x0.02659654E43CF52B1p0", "0x0.026EC8E0694DEC9A9p0", "0x0.02780DBADB0553DA6p0", 
"0x0.028164E8CDB0D8211p0", "0x0.028ACE6ED6C35EB5Fp0", "0x0.02944A518CD54E484p0", "0x0.029DD89587A4D858Dp0", 
"0x0.02A7793F601642B5Dp0", "0x0.02B12C53B03431090p0", "0x0.02BAF1D7132FEE788p0", "0x0.02C4C9CE2561B759Fp0", 
"0x0.02CEB43D844902F7Ap0", "0x0.02D8B129CE8CCD68Fp0", "0x0.02E2C097A3FBE17C9p0", "0x0.02ECE28BA58D22B56p0", 
"0x0.02F7170A755FD759Ep0", "0x0.03015E18B6BBF2966p0", "0x0.030BB7BB0E125EB13p0", "0x0.031623F620FD4751Ep0", 
"0x0.0320A2CE964063DACp0", "0x0.032B344915C941D54p0", "0x0.0335D86A48AF8F70Cp0", "0x0.03408F36D93566140p0", 
"0x0.034B58B372C795013p0", "0x0.035634E4C1FDEC0D0p0", "0x0.036123CF749B8667Cp0", "0x0.036C2578398F157A0p0", 
"0x0.037739E3C0F32BD30p0", "0x0.03826116BC0E882ACp0", "0x0.038D9B15DD5460763p0", "0x0.0398E7E5D864AD0E9p0", 
"0x0.03A4478B620C73EB5p0", "0x0.03AFBA0B304613EEDp0", "0x0.03BB3F69FA3990464p0", "0x0.03C6D7AC783CDBDBFp0", 
"0x0.03D282D763D424DCDp0", "0x0.03DE40EF77B220509p0", "0x0.03EA11F96FB855C4Fp0", "0x0.03F5F5FA08F76B0BBp0", 
"0x0.0401ECF601AF700BDp0", "0x0.040DF6F219502AA55p0", "0x0.041A13F3107962A88p0", "0x0.042643FDA8FB2DDFBp0", 
"0x0.04328716A5D63C2C4p0", "0x0.043EDD42CB3C23B6Dp0", "0x0.044B4686DE8FAD325p0", "0x0.0457C2E7A66520322p0", 
"0x0.04645269EA828F932p0", "0x0.0470F51273E025F89p0", "0x0.047DAAE60CA8725B3p0", "0x0.048A73E98038B4ABFp0", 
"0x0.049750219B212A8A0p0", "0x0.04A43F932B255C0B7p0", "0x0.04B14242FF3C689A0p0", "0x0.04BE5835E79153E1Dp0", 
"0x0.04CB8170B58352D4Ep0", "0x0.04D8BDF83BA618C04p0", "0x0.04E60DD14DC22475Ep0", "0x0.04F37100C0D50D88Ep0", 
"0x0.0500E78B6B11D19D6p0", "0x0.050E717623E121CBAp0", "0x0.051C0EC5C3E1B016Cp0", "0x0.0529BF7F24E87CF6Ap0", 
"0x0.053783A7220124F51p0", "0x0.05455B42976E2E5EAp0", "0x0.0553465662A95706Bp0", "0x0.056144E76263E21F4p0", 
"0x0.056F56FA7686E623Ep0", "0x0.057D7C9480339AD83p0", "0x0.058BB5BA61C3A75A3p0", "0x0.059A0270FEC97047Ap0", 
"0x0.05A862BD3C1065F74p0", "0x0.05B6D6A3FF9D52C58p0", "0x0.05C55E2A30AEA974Ep0", "0x0.05D3F954B7BCD3A1Bp0", 
"0x0.05E2A8287E7A8049Ep0", "0x0.05F16AAA6FD4F267Bp0", "0x0.060040DF77F44FA13p0", "0x0.060F2ACC843BEF0A0p0", 
"0x0.061E2876834AA7FA4p0", "0x0.062D39E264FB20F7Ep0", "0x0.063C5F151A641EB4Cp0", "0x0.064B981395D8D31FEp0", 
"0x0.065AE4E2CAE92C8AAp0", "0x0.066A4587AE6224E1Cp0", "0x0.0679BA07364E10FA3p0", "0x0.0689426659F4EFF1Bp0", 
"0x0.0698DEAA11DCBAA37p0", "0x0.06A88ED757C9B3304p0", "0x0.06B852F326BEB49B0p0", "0x0.06C82B027AFD8278Ap0", 
"0x0.06D8170A520718B46p0", "0x0.06E8170FAA9BFB67Ep0", "0x0.06F82B1784BC86C72p0", "0x0.07085326E1A93F207p0", 
"0x0.07188F42C3E320F0Bp0", "0x0.0728DF702F2BF10B2p0", "0x0.073943B428868CD5Ap0", "0x0.0749BC13B6373A98Fp0", 
"0x0.075A4893DFC3F9E4Cp0", "0x0.076AE939ADF4D4083p0", "0x0.077B9E0A2AD42C9E8p0", "0x0.078C670A61AF122F4p0", 
"0x0.079D443F5F158EE3Ap0", "0x0.07AE35AE30DAF94F1p0", "0x0.07BF3B5BE616454CBp0", "0x0.07D0554D8F2254F0Cp0", 
"0x0.07E183883D9E498E2p0", "0x0.07F2C611046DD4D06p0", "0x0.08041CECF7B989EA2p0", "0x0.081588212CEF2ED73p0", 
"0x0.082707B2BAC20DB40p0", "0x0.08389BA6B92B46284p0", "0x0.084A4402416A1EE6Fp0", "0x0.085C00CA6E045741Fp0", 
"0x0.086DD2045AC678D2Bp0", "0x0.087FB7B524C42936Cp0", "0x0.0891B1E1EA587BE12p0", "0x0.08A3C08FCB2643FFEp0", 
            };
            ap_ufixed<64,-4> f_x_msb_2 = f_x_msb_2_table[x_msb_ind_2];
            ap_ufixed< 6,-4> f_x_msb_2_h;
            f_x_msb_2_h(5,0) = f_x_msb_2(63,58);
            ap_ufixed< 9,-1> exp_x_msb_2_m_1_h = f_x_msb_2_h + x_msb_2;
            ap_ufixed<67,-1> exp_x_msb_2_m_1;
            exp_x_msb_2_m_1(66,58) = exp_x_msb_2_m_1_h(8,0);
            exp_x_msb_2_m_1(57, 0) = f_x_msb_2(57,0);

            ap_ufixed<130,-10> f_x_msb_2_3_4_lsb = exp_x_msb_2_m_1 * exp_x_msb_3_4_lsb_m_1;
            ap_ufixed<62,-10> f_x_msb_2_3_4_lsb_s = f_x_msb_2_3_4_lsb;
            ap_ufixed<72,0> exp_x_msb_2_3_4_lsb_m_1 = f_x_msb_2_3_4_lsb_s + exp_x_msb_2_m_1 + exp_x_msb_3_4_lsb_m_1;

            const ap_ufixed<68,32> exp_x_msb_1_table[256] = {
"0x1.000000000p0", "0x1.48B5E3C3Ep0", "0x1.A61298E1Ep0", "0x2.1DF3B68CFp0", 
"0x2.B7E151628p0", "0x3.7D871DB61p0", "0x4.7B4FF993Fp0", "0x5.C12DA416Ep0", 
"0x7.63992E353p0", "0x9.7CDC417A3p0", "0xC.2EB7EC98Fp0", "0xF.A48385EDFp0", 
"0x14.15E5BF6FBp0", "0x19.CA53B7811p0", "0x21.1D8E4272Dp0", "0x2A.8565A144Cp0", 
"0x36.99205C4E7p0", "0x46.1AFC4DB59p0", "0x5A.0462B7877p0", "0x73.9593ABB7Dp0", 
"0x94.69C4CB819p0", "0xBE.90F6F83E9p0", "0xF4.B122790DDp0", "0x13A.30CF1CCBBp0", 
"0x193.6DC5690C0p0", "0x206.03487A3B1p0", "0x299.2442102D9p0", "0x356.0F0B0F980p0", 
"0x448.A216ABB76p0", "0x580.1AD754FA3p0", "0x710.0ADBAC7DAp0", "0x911.9289C3923p0", 
"0xBA4.F53EA3863p0", "0xEF3.A035D5798p0", "0x1332.C4D2B7C4Ap0", "0x18A6.B027DA0A7p0", 
"0x1FA7.157C470F8p0", "0x28A4.90D2CCEF1p0", "0x342F.BA11823B8p0", "0x4302.3A933EE5Cp0", 
"0x560A.773E54157p0", "0x6E7A.8ABB4A83Cp0", "0x8DDB.80AF4269Dp0", "0xB626.0748BAEC7p0", 
"0xE9E2.2447727BFp0", "0x12C4F.EB75B2AB0p0", "0x1819B.C560F6113p0", "0x1EF21.8F1B001A4p0", 
"0x27BC2.CA9A6F934p0", "0x33055.49F35D91Fp0", "0x41831.49596E996p0", "0x541E7.E56949D58p0", 
"0x6C02D.645AB2554p0", "0x8AB06.0A3EE9FB1p0", "0xB2148.5EAE56C5Bp0", "0xE4A8D.2881EDBE8p0", 
"0x1259AC.48BF05D6Ep0", "0x178FEE.7792E44B3p0", "0x1E4127.437732B71p0", "0x26D8F9.4A204BEA5p0", 
"0x31E199.5F5A550DDp0", "0x400C7D.64D3386CBp0", "0x523D82.79EDAEF9Cp0", "0x69993D.D4F2D9524p0", 
"0x87975E.854001024p0", "0xAE1A47.C38A42CD0p0", "0xDF8D5F.6DCFE5CEAp0", "0x11F0BF8.1E99FDEB6p0", 
"0x1709348.C0EA4F8CEp0", "0x1D94295.464477ACCp0", "0x25FAD90.65C78E342p0", "0x30C4623.616ED2BE2p0", 
"0x3E9E441.232817A61p0", "0x506744D.3B342FC94p0", "0x673D70B.C00F1F76Dp0", "0x849012B.C127FC962p0", 
"0xAA36C7C.F69370B94p0", "0xDA8F202.DD9ACE661p0", "0x118A2AAE.0AA05236Bp0", "0x16857CFA.1AA8A5555p0", 
"0x1CEB088B.68E804021p0", "0x2521AEDA.32CD52BE5p0", "0x2FAD89E1.79BAB8EADp0", "0x3D3838C6.BE0F13F06p0", 
"0x4E9B87F6.7BB3F5597p0", "0x64EF201B.01109A4A5p0", "0x819A1801.9394C0D96p0", "0xA6698403.CF5D9E663p0", 
"0xD5AD6DCE.21AFFC0DDp0", "0x1125E066B.0B19418CCp0", "0x1604B68CF.05F65FAA9p0", "0x1C45AED0A.D0C934CDCp0", 
"0x244D5E676.3FB814AF0p0", "0x2E9CEBF49.9C21839C7p0", "0x3BDA2CBD0.9F5AC2146p0", "0x4CDA0FD41.989F004E7p0", 
"0x62ADFED51.D7EDCCDE6p0", "0x7EB50B520.76F91D6E8p0", "0xA2B1FD3D9.A57B6C33Cp0", "0xD0E7A50F4.EF0D7447Fp0", 
"0x10C3D39209.62C88AAFBp0", "0x1586D0722B.3EEB27301p0", "0x1BA4068AAC.2B6E3EE34p0", "0x237DCBF1E5.709A9F9C1p0", 
"0x2D9264D2D4.D6B94153Bp0", "0x3A83F23B18.FE5BD524Ap0", "0x4B22A1B5C3.27A2FD85Ap0", "0x6079C1866C.71A93951Dp0", 
"0x7BE08BDFD9.CC72249ABp0", "0x9F0FB7309C.B446D3919p0", "0xCC3D265776.C3D25779Ep0", "0x1063F7612B0.C0831CA6Cp0", 
"0x150BBA37963.79A4FB6ADp0", "0x1B05FA9B62E.737D935B8p0", "0x22B2DC5C41D.AD89559EBp0", "0x2C8DD1AB28B.404414BE7p0", 
"0x39355C8C638.0AC5631B1p0", "0x49750434675.301B8D0E7p0", "0x5E521E7A16E.51F9CD3C2p0", "0x791C3B06F8B.EA2DEC48Cp0", 
"0x9B823857614.764F43E20p0", "0xC7AD559D438.E7C270C30p0", "0x10063F4E687B.A1A43A35Bp0", "0x149363C905BA.3641D0164p0", 
"0x1A6B765D8CDF.6CDBF1C63p0", "0x21EC75240E66.E2891CE63p0", "0x2B8F1073A6AF.BA219DB61p0", "0x37EE3FFC0063.E360F931Fp0", 
"0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", 
"0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", 
"0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", 
"0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", 
"0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", 
"0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", 
"0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", "0x0.000000000p0", 
"0x0.000000000p0", "0x0.000000001p0", "0x0.000000001p0", "0x0.000000002p0", 
"0x0.000000002p0", "0x0.000000003p0", "0x0.000000004p0", "0x0.000000005p0", 
"0x0.000000007p0", "0x0.000000009p0", "0x0.00000000Bp0", "0x0.00000000Ep0", 
"0x0.000000013p0", "0x0.000000018p0", "0x0.00000001Fp0", "0x0.000000028p0", 
"0x0.000000034p0", "0x0.000000042p0", "0x0.000000055p0", "0x0.00000006Ep0", 
"0x0.00000008Dp0", "0x0.0000000B5p0", "0x0.0000000E9p0", "0x0.00000012Bp0", 
"0x0.000000181p0", "0x0.0000001EEp0", "0x0.00000027Ap0", "0x0.00000032Fp0", 
"0x0.000000416p0", "0x0.00000053Fp0", "0x0.0000006BDp0", "0x0.0000008A7p0", 
"0x0.000000B1Cp0", "0x0.000000E44p0", "0x0.000001252p0", "0x0.000001786p0", 
"0x0.000001E35p0", "0x0.0000026C9p0", "0x0.0000031CEp0", "0x0.000003FF3p0", 
"0x0.00000521Dp0", "0x0.000006970p0", "0x0.000008762p0", "0x0.00000ADD6p0", 
"0x0.00000DF36p0", "0x0.000011E9Cp0", "0x0.000017003p0", "0x0.00001D88Ap0", 
"0x0.000025EC0p0", "0x0.000030B15p0", "0x0.00003E85Dp0", "0x0.00005047Ep0", 
"0x0.000067153p0", "0x0.0000845C6p0", "0x0.0000A9F46p0", "0x0.0000DA39Ep0", 
"0x0.000118354p0", "0x0.000167CB5p0", "0x0.0001CDFC2p0", "0x0.000251334p0", 
"0x0.0002F9AF3p0", "0x0.0003D205Ap0", "0x0.0004E7CE2p0", "0x0.00064C7C6p0", 
"0x0.000816791p0", "0x0.000A628A3p0", "0x0.000D55A1Fp0", "0x0.00111F30Fp0", 
"0x0.0015FC210p0", "0x0.001C3AA91p0", "0x0.00243F374p0", "0x0.002E8ABFCp0", 
"0x0.003BC2D73p0", "0x0.004CBC19Ap0", "0x0.006287862p0", "0x0.007E83A56p0", 
"0x0.00A2728F8p0", "0x0.00D096336p0", "0x0.010BD4A5Ap0", "0x0.0157E6BFEp0", 
"0x0.01B993FE0p0", "0x0.0236FF5BFp0", "0x0.02D80A08Dp0", "0x0.03A6D2222p0", 
"0x0.04B0556E0p0", "0x0.0605424D5p0", "0x0.07BB04063p0", "0x0.09ED1B429p0", 
"0x0.0CBED8666p0", "0x0.105D93892p0", "0x0.150385C09p0", "0x0.1AFB718E8p0", 
"0x0.22A555477p0", "0x0.2C7C72FC0p0", "0x0.391F0EE49p0", "0x0.495860DCAp0", 
"0x0.5E2D58D8Bp0", "0x0.78ED03AFBp0", "0x0.9B4597E37p0", "0x0.C75F7CF56p0", 
            };
            ap_ufixed<68,32> exp_x_msb_1 = exp_x_msb_1_table[x_msb_ind_1];
            ap_ufixed<140,32> y_lo = exp_x_msb_1 * exp_x_msb_2_3_4_lsb_m_1;
            ap_ufixed<68,32> y_lo_s = y_lo;
            ap_ufixed<68,32> y_l = y_lo_s + exp_x_msb_1;
            y = y_l;
        }
        if (I_<33) { // overflow
            bool overf = 0;
            #pragma unroll
            for (int j = 63; j >= I_+31; j--) {
                if (y[j])
                    overf = 1;
            }
            if (overf) {
                #pragma unroll
                for (int j = 63; j >= I_+31; j--)
                    y[j] = 0;
                #pragma unroll
                for (int j = I_+30; j >= 0; j--)
                    y[j] = 1;
            }
        }
        r = y;
    }
    return r;
}

template<int W_, int I_>
ap_ufixed<W_,I_> exp(ap_ufixed<W_,I_> x) {
    ap_fixed<W_+1,I_+1> xf = x;
    return exp(xf);
}

template<int I_>
ap_int<I_> exp(ap_int<I_> x) {
    ap_fixed<I_,I_> xf = x;
    return exp(xf);
}

template<int I_>
ap_uint<I_> exp(ap_uint<I_> x) {
    ap_fixed<I_+1,I_+1> xf = x;
    return exp(xf);
}


template<typename T>
T exp2_generic(T x)
{
#pragma HLS pipeline

	fp_struct<T> es(x);
        const static int we = exp_traits<T>::we;
        const static int wf = exp_traits<T>::wf;

        fp_struct<T> out;
	out.sign[0] = 0;
	out.sig = 0;

// special cases include:
// y = 0, +inf, -inf, NaN
//
//      x	|       +inf	|	-inf	|       NaN     
// =============================================================
// 	exp(x)	|	+inf	|	0	|	NaN	
//
    bool x_is_NaN = 0;
    bool x_is_inf = 0;
    bool x_is_pinf = 0;
    bool x_is_ninf = 0;

    if ( ::hlstmp::__isnan(x) )    x_is_NaN = 1;
    if ( ::hlstmp::__isinf(x) )	x_is_inf = 1;
    x_is_pinf = x_is_inf & ~es.sign[0];
    x_is_ninf = x_is_inf & es.sign[0];

    if (x_is_NaN) {
        // out = NaN
        //out.sign[0] = 0;
        out.sig = -1; // all 1's
        out.exp = fp_struct<T>::EXP_INFNAN;
        return out.to_ieee();
    }
    if (x_is_pinf) {
	// out = +inf
	//out.sign[0] = 0;
	//out.sig = 0;
	out.exp = fp_struct<T>::EXP_INFNAN;
	return out.to_ieee();
    }
    if (x_is_ninf) {
        // out = 0
        //out.sign[0] = 0;
        //out.sig = 0;
        out.exp = 0;
        return out.to_ieee();
    }


    int m_exp = es.expv();
    fp_struct<T> nes = es;
#ifdef STDSUBNORMALS
    if(nes.exp == 0 && nes.sig != 0) {
        // subnormal handling.
        unsigned int zeros;
        #pragma unroll
        for (zeros = 0; zeros < wf; zeros++)
            if ( nes.sig[wf-zeros-1] == 1 ) break;
        m_exp -= zeros;
        nes.sig = nes.sig << (zeros + 1); // add one so we shift off the leading one
    }
#endif
    ap_fixed<1 + 1 + wf, 1 + 1> e_frac = 0;
    e_frac[e_frac.wl()-e_frac.iwl()] = 1; // The implicit '1' in IEEE format.
    e_frac(e_frac.wl()-e_frac.iwl()-1,0) = nes.sig(wf-1,0);
    if (nes.sign) e_frac = -e_frac;

    const static int gbits = exp_traits<T>::gbits;
    ap_fixed<1 + we + wf+gbits, 1 + we> m_frac_l = e_frac;

    ap_fixed<1 + we + wf, 1 + we> m_fix_l = m_frac_l << m_exp; // used for overflow checking only
    ap_fixed<1 + we + wf, 1 + we> m_fix_back = m_fix_l >> m_exp;

    ap_fixed<1 + we + wf+gbits, 1 + we> m_fix;
    m_fix = m_frac_l << m_exp;

    ap_fixed<2, 1> delta1;
    delta1[1] = m_fix[m_fix.wl()-1];
    delta1[0] = 1;

    ap_int<1 + we+1> r_exp = m_fix + delta1;

    const ap_ufixed<wf+gbits+we+1, 0> LOG2_hi = 0.69314718055994517520446152047953;
    const ap_ufixed<wf+gbits+we+1, 0> LOG2_lo = 0.60444058366692929317548838826088;
    const ap_ufixed<wf+gbits+we+1, 0> LOG2 = LOG2_hi + ( LOG2_lo >> 52 );

    ap_fixed<1 + we + wf+gbits, 1 + we+1> m_fix_a = r_exp;// m_fix approximation

    assert( (m_fix_back != m_frac_l) || (m_fix - m_fix_a < 1) );// check r_exp zeros out integer and most significant fraction bits
    assert( (m_fix_back != m_frac_l) || (m_fix - m_fix_a > -1) );// check r_exp zeros out integer and most significant fraction bits
    ap_fixed<1 + wf+gbits, 1> m_diff_l = ( m_fix - m_fix_a ) * LOG2;

    assert( (m_fix_back != m_frac_l) || (m_diff_l < 0.5) );// check r_exp zeros out integer and most significant fraction bits
    assert( (m_fix_back != m_frac_l) || (m_diff_l > -0.5) );// check r_exp zeros out integer and most significant fraction bits
    ap_fixed<1 -1 + wf+gbits, 1 -1> m_diff = m_diff_l;

    // e^Y = 1 + Y + Y^2/2 + ... + Y^n/n! + ...
    // term Y^n/n! can be eliminated when its MSB is less than 2^-(wf+g)
    // Y belongs to (-.5,.5)
    // w = wf+g
    // g = 3,4,7 for h,f,d
    // g_Z2 = 3,3,5 for h,f,d
    // Y = Z1 + Z1P
    //   = Z1 + Z2 + Z2P
    //   = ...
    //   = Z1 + Z2 + ... + Zk
    // wn is width of Zn, n = 1...k
    // T_Z1 = 2^w1*(w+1)
    // T_Z2 = 2^w2*(w+1-2*w1)
    // T_Z3 = 2^w3*(w+1-2*(w1+w2))
    // ...
    //
    //		|	h	|	f	|	d	
    //	========================================================
    //	wf	|	10	|	23	|	52	
    //	g	|	3	|	4	|	7	
    //	g_Z2    |       3       |       3       |       5
    //	w	|	13	|	27	|	59	
    //	k	|	2	|	2	|	4	
    //	wn	|	9,4	|	9,18	|    8,8,8,35	
    //	T_total	|	7k	|	<18k	| 14.5k+10.5k+6.5k
    //	Mult	|	5bit	|	1DSP	|	16DSP	

    const static int w_Z1 = exp_traits<T>::w_Z1;
    // Z1
    ap_uint<w_Z1> m_diff_hi = m_diff ( m_diff.wl()-1 , m_diff.wl()-w_Z1 );
    // Z1P = Z2 + ... + Zk
    ap_ufixed<wf+gbits-w_Z1, -w_Z1> m_diff_lo = m_diff; // ( m_diff.wl()-m_diff.iwl()-w_Z1-1 , 0 );

    // e^Z1 by table_exp_Z1
    const static int gbits_Z2 = exp_traits<T>::gbits_Z2;
    ap_ufixed<1 + wf+gbits_Z2, 1> exp_Z1 = table_exp_Z1< ap_ufixed<1 + wf+gbits_Z2, 1> >::array [ m_diff_hi ];
    ap_ufixed<wf+gbits_Z2-w_Z1+1, -w_Z1+1> exp_Z1P_m_1 = exp_traits<T>::exp_Z1P_m_1 ( m_diff_lo );
    ap_ufixed<1 + wf+gbits_Z2-w_Z1, 1> exp_Z1_hi = exp_Z1;
    ap_ufixed<1, -wf> delta;
    delta[0] = 1;
    ap_ufixed<2 + wf+gbits_Z2, 2> exp_Y_l = ( exp_Z1 + delta ) + exp_Z1_hi * exp_Z1P_m_1;
    assert ( exp_Y_l[exp_Y_l.wl()-1] == 0 );
    ap_ufixed<1 + wf+gbits_Z2, 1> exp_Y = exp_Y_l;

    if ( exp_Y[exp_Y.wl()-1] == 0 ) {
        exp_Y = exp_Y << 1;
        r_exp = r_exp - 1;
    }

    // check overflow here
    if ( ( m_exp > 0 && m_fix_back != m_frac_l ) || ( r_exp > fp_struct<T>::EXP_BIAS ) ) {
	if ( ~m_frac_l[m_frac_l.wl()-1] ) {
	// out = +inf
	//out.sign[0] = 0;
	//out.sig = 0;
            out.exp = fp_struct<T>::EXP_INFNAN;
            return out.to_ieee();
	} else {
	// out = 0
	//out.sign[0] = 0;
	//out.sig = 0;
            out.exp = 0;
            return out.to_ieee();
	}
    }

    // check underflow here
    if ( r_exp <= -fp_struct<T>::EXP_BIAS ) {
        // out = 0
        //out.sign[0] = 0;
        //out.sig = 0;
        out.exp = 0;
        return out.to_ieee();
    }

// let's output the result
    out.sig(wf-1,0) = exp_Y ( exp_Y.wl()-1-1 , exp_Y.wl()-1-wf );
    out.exp = fp_struct<T>::EXP_BIAS+r_exp;
    return out.to_ieee();

}

static half exp2(half x)
{
	return exp2_generic(x);
}
static float exp2(float x)
{
	return exp2_generic(x);
}
static double exp2(double x)
{
	return exp2_generic(x);
}

static float exp2f(float x)
{
	return exp2_generic(x);
}

static half half_exp2(half x)
{
	return exp2_generic(x);
}


template<typename T>
T exp10_generic(T x)
{
#pragma HLS pipeline

	fp_struct<T> es(x);
        const static int we = exp_traits<T>::we;
        const static int wf = exp_traits<T>::wf;

        fp_struct<T> out;
	out.sign[0] = 0;
	out.sig = 0;

// special cases include:
// y = 0, +inf, -inf, NaN
//
//      x	|       +inf	|	-inf	|       NaN     
// =============================================================
// 	exp(x)	|	+inf	|	0	|	NaN	
//
    bool x_is_NaN = 0;
    bool x_is_inf = 0;
    bool x_is_pinf = 0;
    bool x_is_ninf = 0;

    if ( ::hlstmp::__isnan(x) )    x_is_NaN = 1;
    if ( ::hlstmp::__isinf(x) )	x_is_inf = 1;
    x_is_pinf = x_is_inf & ~es.sign[0];
    x_is_ninf = x_is_inf & es.sign[0];

    if (x_is_NaN) {
        // out = NaN
        //out.sign[0] = 0;
        out.sig = -1; // all 1's
        out.exp = fp_struct<T>::EXP_INFNAN;
        return out.to_ieee();
    }
    if (x_is_pinf) {
	// out = +inf
	//out.sign[0] = 0;
	//out.sig = 0;
	out.exp = fp_struct<T>::EXP_INFNAN;
	return out.to_ieee();
    }
    if (x_is_ninf) {
        // out = 0
        //out.sign[0] = 0;
        //out.sig = 0;
        out.exp = 0;
        return out.to_ieee();
    }


    int m_exp = es.expv();
    fp_struct<T> nes = es;
#ifdef STDSUBNORMALS
    if(nes.exp == 0 && nes.sig != 0) {
        // subnormal handling.
        unsigned int zeros;
        #pragma unroll
        for (zeros = 0; zeros < wf; zeros++)
            if ( nes.sig[wf-zeros-1] == 1 ) break;
        m_exp -= zeros;
        nes.sig = nes.sig << (zeros + 1); // add one so we shift off the leading one
    }
#endif
    ap_fixed<1 + 1 + wf, 1 + 1> e_frac = 0;
    e_frac[e_frac.wl()-e_frac.iwl()] = 1; // The implicit '1' in IEEE format.
    e_frac(e_frac.wl()-e_frac.iwl()-1,0) = nes.sig(wf-1,0);
    if (nes.sign) e_frac = -e_frac;

    const static int gbits = exp_traits<T>::gbits;
    ap_fixed<1 + we + wf+gbits, 1 + we> m_frac_l = e_frac;

    ap_fixed<1 + we + wf, 1 + we> m_fix_l = m_frac_l << m_exp; // used for overflow checking only
    ap_fixed<1 + we + wf, 1 + we> m_fix_back = m_fix_l >> m_exp;

    ap_fixed<1 + we + wf+gbits, 1 + we> m_fix;
    m_fix = m_frac_l << m_exp;

    ap_fixed<1 + we + 5, 1 + we> m_fix_hi = m_fix;

    const ap_ufixed<2 + we+3, 2> LOG2_10 = 3.3219280948873623478703194294894;

    ap_fixed<2, 1> delta1;
    delta1[1] = m_fix[m_fix.wl()-1];
    delta1[0] = 1;

    ap_int<1 + we+2> r_exp = m_fix_hi * LOG2_10 + delta1;

    const ap_ufixed< 2 + wf+gbits+we+1,  2> LOG10_hi = 2.302585092994045679049008867878;
    const ap_ufixed<-5 + wf+gbits+we+1, -5> LOG10_lo = 0.02237830812635162513896609861137;
    const ap_ufixed< 2 + wf+gbits+we+1,  2> LOG10 = LOG10_hi + ( LOG10_lo >> 52 );

    const ap_ufixed<-1 + wf+gbits+we+3, -1> LOG10_2_hi = 0.30102999566398103148401332873618;
    const ap_ufixed<     wf+gbits+we+3,  0> LOG10_2_lo = 0.73737313104845853883700312679039;
    const ap_ufixed<-1 + wf+gbits+we+3, -1> LOG10_2 = LOG10_2_hi + ( LOG10_2_lo >> 52 );

    ap_fixed<1 + we+1 + wf+gbits, 1 + we+1> m_fix_a = r_exp * LOG10_2;// m_fix approximation

    assert( (m_fix_back != m_frac_l) || (m_fix - m_fix_a < 0.25) );// check r_exp zeros out integer and most significant fraction bits
    assert( (m_fix_back != m_frac_l) || (m_fix - m_fix_a > -0.25) );// check r_exp zeros out integer and most significant fraction bits
    ap_fixed<1 -1 + wf+gbits, 1 -1> m_diff = ( m_fix - m_fix_a ) * LOG10;

    // e^Y = 1 + Y + Y^2/2 + ... + Y^n/n! + ...
    // term Y^n/n! can be eliminated when its MSB is less than 2^-(wf+g)
    // Y belongs to (-.5,.5)
    // w = wf+g
    // g = 3,4,7 for h,f,d
    // g_Z2 = 3,3,5 for h,f,d
    // Y = Z1 + Z1P
    //   = Z1 + Z2 + Z2P
    //   = ...
    //   = Z1 + Z2 + ... + Zk
    // wn is width of Zn, n = 1...k
    // T_Z1 = 2^w1*(w+1)
    // T_Z2 = 2^w2*(w+1-2*w1)
    // T_Z3 = 2^w3*(w+1-2*(w1+w2))
    // ...
    //
    //		|	h	|	f	|	d	
    //	========================================================
    //	wf	|	10	|	23	|	52	
    //	g	|	3	|	4	|	7	
    //	g_Z2    |       3       |       3       |       5
    //	w	|	13	|	27	|	59	
    //	k	|	2	|	2	|	4	
    //	wn	|	9,4	|	9,18	|    8,8,8,35	
    //	T_total	|	7k	|	<18k	| 14.5k+10.5k+6.5k
    //	Mult	|	5bit	|	1DSP	|	16DSP	

    const static int w_Z1 = exp_traits<T>::w_Z1;
    // Z1
    ap_uint<w_Z1> m_diff_hi = m_diff ( m_diff.wl()-1 , m_diff.wl()-w_Z1 );
    // Z1P = Z2 + ... + Zk
    ap_ufixed<wf+gbits-w_Z1, -w_Z1> m_diff_lo = m_diff; // ( m_diff.wl()-m_diff.iwl()-w_Z1-1 , 0 );

    // e^Z1 by table_exp_Z1
    const static int gbits_Z2 = exp_traits<T>::gbits_Z2;
    ap_ufixed<1 + wf+gbits_Z2, 1> exp_Z1 = table_exp_Z1< ap_ufixed<1 + wf+gbits_Z2, 1> >::array [ m_diff_hi ];
    ap_ufixed<wf+gbits_Z2-w_Z1+1, -w_Z1+1> exp_Z1P_m_1 = exp_traits<T>::exp_Z1P_m_1 ( m_diff_lo );
    ap_ufixed<1 + wf+gbits_Z2-w_Z1, 1> exp_Z1_hi = exp_Z1;
    ap_ufixed<1, -wf> delta;
    delta[0] = 1;
    ap_ufixed<2 + wf+gbits_Z2, 2> exp_Y_l = ( exp_Z1 + delta ) + exp_Z1_hi * exp_Z1P_m_1;
    assert ( exp_Y_l[exp_Y_l.wl()-1] == 0 );
    ap_ufixed<1 + wf+gbits_Z2, 1> exp_Y = exp_Y_l;

    if ( exp_Y[exp_Y.wl()-1] == 0 ) {
        exp_Y = exp_Y << 1;
        r_exp = r_exp - 1;
    }

    // check overflow here
    if ( ( m_exp > 0 && m_fix_back != m_frac_l ) || ( r_exp > fp_struct<T>::EXP_BIAS ) ) {
	if ( ~m_frac_l[m_frac_l.wl()-1] ) {
	// out = +inf
	//out.sign[0] = 0;
	//out.sig = 0;
            out.exp = fp_struct<T>::EXP_INFNAN;
            return out.to_ieee();
	} else {
	// out = 0
	//out.sign[0] = 0;
	//out.sig = 0;
            out.exp = 0;
            return out.to_ieee();
	}
    }

    // check underflow here
    if ( r_exp <= -fp_struct<T>::EXP_BIAS ) {
        // out = 0
        //out.sign[0] = 0;
        //out.sig = 0;
        out.exp = 0;
        return out.to_ieee();
    }

// let's output the result
    out.sig(wf-1,0) = exp_Y ( exp_Y.wl()-1-1 , exp_Y.wl()-1-wf );
    out.exp = fp_struct<T>::EXP_BIAS+r_exp;
    return out.to_ieee();

}

static half exp10(half x)
{
	return exp10_generic(x);
}
static float exp10(float x)
{
	return exp10_generic(x);
}
static double exp10(double x)
{
	return exp10_generic(x);
}

static float exp10f(float x)
{
	return exp10_generic(x);
}

static half half_exp10(half x)
{
	return exp10_generic(x);
}


static double expm1(double x)
{
        fp_struct<double> xs(x);
        if ( (xs.sign[0]==1) && (xs.exp<996) && (xs.exp!=0) ) return x;
	if ( (xs.sign[0]==0) && (xs.exp<=996) && (xs.exp!=0) ) return x;
	return exp_generic(x)-1;
}

static float expm1(float x)
{
        fp_struct<float> xs(x);
	if ( xs.exp == 0 ) return 0;
        if ( xs.exp < 96 ) return x;
	double xd = x;
	return exp_generic(xd)-1;
}

static half expm1(half x)
{
	float xf = x;
	return exp_generic(xf)-1;
}

}
