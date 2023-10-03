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

    amrex::ParticleReal ds = 0._prt;
    amrex::ParticleReal bscale = 0._prt;
    std::vector<amrex::ParticleReal> sin_coef = {};
    std::vector<amrex::ParticleReal> cos_coef = {};
    int mapsteps = 1;
    int nslice = 1;

    utils::parser::queryWithParser(pp_element, "ds", ds);
    utils::parser::queryWithParser(pp_element, "bscale", bscale);
    utils::parser::queryWithParser(pp_element, "sin_coef", sin_coef);
    utils::parser::queryWithParser(pp_element, "cos_coef", cos_coef);
    utils::parser::queryWithParser(pp_element, "mapsteps", mapsteps);
    utils::parser::queryWithParser(pp_element, "nslice", nslice);

    h_ds.push_back(ds);
    h_bscale.push_back(bscale);
    h_sin_coef.push_back(sin_coef);
    h_cos_coef.push_back(cos_coef);
    h_mapsteps.push_back(mapsteps);
    h_nslice.push_back(nslice);

}

void
Solenoid::WriteToDevice ()
{
    WriteToDeviceBase();

    d_ds.resize(h_ds.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_ds.begin(), h_ds.end(), d_ds.begin());
    d_bscale.resize(h_bscale.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_bscale.begin(), h_bscale.end(), d_bscale.begin());
    d_sin_coef.resize(h_sin_coef.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_sin_coef.begin(), h_sin_coef.end(), d_sin_coef.begin());
    d_cos_coef.resize(h_cos_coef.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_cos_coef.begin(), h_cos_coef.end(), d_cos_coef.begin());
    d_mapsteps.resize(h_ds.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_mapsteps.begin(), h_mapsteps.end(), d_mapsteps.begin());
    d_nslice.resize(h_nslice.size());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice, h_nslice.begin(), h_nslice.end(), d_nslice.begin());
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

    d_ds_arr = h_solenoid.d_ds.data();
    d_bscale_arr = h_solenoid.d_bscale.data();
    d_sin_coef_arr = h_solenoid.d_sin_coef.data();
    d_cos_coef_arr = h_solenoid.d_cos_coef.data();
    d_mapsteps_arr = h_solenoid.d_mapsteps.data();
    d_nslice_arr = h_solenoid.d_nslice.data();

}
