/* Copyright 2023 David Grote and David Bizzozero
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "Solenoid.H"
#include "Utils/Parser/ParserUtils.H"

#include <AMReX_ParmParse.H>
#include <AMReX_REAL.H>

#include <string>

Solenoid::Solenoid ()
    : LatticeElementBase("solenoid")
{
}

void
Solenoid::AddElement (amrex::ParmParse & pp_element, amrex::ParticleReal & z_location)
{
    using namespace amrex::literals;

    AddElementBase(pp_element, z_location);

    amrex::ParticleReal scale = 0._prt;
    amrex::ParticleReal freq = 0._prt;
    amrex::ParticleReal theta = 0._prt;
    amrex::Vector<amrex::ParticleReal> e_coef = {};
    amrex::Vector<amrex::ParticleReal> b_coef = {};

    utils::parser::queryWithParser(pp_element, "scale", scale);
    utils::parser::queryWithParser(pp_element, "freq", freq);
    utils::parser::queryWithParser(pp_element, "theta", theta);
    utils::parser::queryArrWithParser(pp_element, "e_coef", e_coef);
    utils::parser::queryArrWithParser(pp_element, "b_coef", b_coef);

    h_scale.push_back(scale);
    h_freq.push_back(freq);
    h_theta.push_back(theta);
    h_e_coef.push_back(e_coef);
    h_b_coef.push_back(b_coef);

}

void
Solenoid::WriteToDevice ()
{
    WriteToDeviceBase();

    d_scale.resize(h_scale.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_scale.begin(), h_scale.end(), d_scale.begin());
    d_freq.resize(h_freq.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_freq.begin(), h_freq.end(), d_freq.begin());
    d_theta.resize(h_theta.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_theta.begin(), h_theta.end(), d_theta.begin());
    d_e_coef.resize(h_e_coef.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_e_coef.begin(), h_e_coef.end(), d_e_coef.begin());
    d_b_coef.resize(h_b_coef.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_b_coef.begin(), h_b_coef.end(), d_b_coef.begin());

}

SolenoidDevice
Solenoid::GetDeviceInstance () const
{
    SolenoidDevice result;
    result.InitSolenoidDevice(*this);
    return result;
}

void
SolenoidDevice::InitSolenoidDevice (Solenoid const& h_solenoid)
{

    nelements = h_solenoid.nelements;

    if (nelements == 0) return;

    d_zs_arr = h_solenoid.d_zs.data();
    d_ze_arr = h_solenoid.d_ze.data();

    d_scale_arr = h_solenoid.d_scale.data();
    d_freq_arr = h_solenoid.d_freq.data();
    d_theta_arr = h_solenoid.d_theta.data();
    d_e_coef_arr = h_solenoid.d_e_coef.data();
    d_b_coef_arr = h_solenoid.d_b_coef.data();

}
