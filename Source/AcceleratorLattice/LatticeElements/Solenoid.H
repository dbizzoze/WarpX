/* Copyright 2023 David Grote and David Bizzozero
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_ACCELERATORLATTICE_LATTICEELEMENTS_SOLENOID_H_
#define WARPX_ACCELERATORLATTICE_LATTICEELEMENTS_SOLENOID_H_

#include "LatticeElementBase.H"
#include "HardEdged_K.H"
#include "Utils/WarpXConst.H"

#include <AMReX_REAL.H>
#include <AMReX_ParmParse.H>
#include <AMReX_GpuContainers.H>

#include <string>
#include <cmath>

// Specifies a field that models the field generated by a solenoidal field,
// an axisymmetric field expansion

struct SolenoidDevice;

struct Solenoid
    : LatticeElementBase
{

    Solenoid ();

    ~Solenoid () = default;

    /**
     * \brief Read in an element and add it to the lists
     *
     * @param[in] pp_element The ParmParse instance to read in the data
     * @param[inout] z_location The current z location in the lattice
     */
    void
    AddElement (amrex::ParmParse & pp_element, amrex::ParticleReal & z_location);

    /**
     * \brief Write the element information to the device
     */
    void
    WriteToDevice ();

    // Host variables read from input file
    std::vector<amrex::ParticleReal> h_scale;
    std::vector<std::vector<amrex::ParticleReal>> h_b_coef;

    // Device variable read from input files
    amrex::Gpu::DeviceVector<amrex::ParticleReal> d_scale;
    amrex::Gpu::DeviceVector<amrex::Vector<amrex::ParticleReal> *> d_b_coef;

    /**
     * \brief Returns the device level instance with the lattice information
     */
    SolenoidDevice GetDeviceInstance () const;


};

// Instance that is trivially copyable to the device.

struct SolenoidDevice
{

    /**
     * \brief Initializes the data and pointer needed to reference the lattice element info
     *
     * @param[in] h_solenoid host level instance that this is associated with
     */
    void InitSolenoidDevice (Solenoid const& h_solenoid);

    int nelements = 0;

    const amrex::ParticleReal* AMREX_RESTRICT d_zs_arr;
    const amrex::ParticleReal* AMREX_RESTRICT d_ze_arr;
    const amrex::ParticleReal* AMREX_RESTRICT d_scale_arr;
    unsigned long* AMREX_RESTRICT d_num_b_coef;
    const amrex::ParticleReal** AMREX_RESTRICT d_b_coef_arr;

    /**
     * \brief Fetch the field of the specified element at the given location
     *
     * @param[in] ielement the element number
     * @param[in] x, y, z the particle position in the lab frame
     * @param[in] m_time the current time in the lab frame
     * @param[out] Ex, Ey, Ez, Bx, By, Bz the fetched field in the lab frame
     */
    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    void get_field (const int ielement,
                    const amrex::ParticleReal x,
                    const amrex::ParticleReal y,
                    const amrex::ParticleReal z,
                    amrex::ParticleReal& Ex,
                    amrex::ParticleReal& Ey,
                    amrex::ParticleReal& Ez,
                    amrex::ParticleReal& Bx,
                    amrex::ParticleReal& By,
                    amrex::ParticleReal& Bz) const
    {

        const amrex::ParticleReal zs = d_zs_arr[ielement];
        const amrex::ParticleReal ze = d_ze_arr[ielement];
        const amrex::ParticleReal scale = d_scale_arr[ielement];
        const amrex::ParticleReal* b_coef = d_b_coef_arr[ielement];

        constexpr amrex::ParticleReal pi = MathConst::pi;   // Everyone's favorite circle ratio

        const int n_coef = (d_num_b_coef[ielement]-1) / 2;  // Number of coef. is half the length of the list: zero and odd inds are cosine, even inds are sine
        const amrex::ParticleReal zlen = ze - zs;           // Length of element

        amrex::ParticleReal zz = z - ze + zlen/2.0;         // Shifted coordinate to midpoint of element (zz = 0 is midpoint of element)
        amrex::ParticleReal bz = b_coef[0]/2.0;             // A_0 cosine fourier coef. is halved (sine coef. is trivially zero)
        amrex::ParticleReal bzp = 0.0;                      // Variable to store Bz'(zz) (z-derivative of on-axis longitudinal field at z = zz)
        amrex::ParticleReal bzpp = 0.0;                     // Variable to store Bz''(zz)
        amrex::ParticleReal bzppp = 0.0;                    // Variable to store Bz'''(zz)
        for (int k = 1; k < n_coef; ++k){
            bz = bz + b_coef[2*k-1]*cos(2*pi*k*zz/zlen) + b_coef[2*k]*sin(2.0*pi*k*zz/zlen);
            bzp = bzp - (2.0*pi*k*zz/zlen)*b_coef[2*k-1]*sin(2.0*pi*k*zz/zlen) + (2.0*pi*k*zz/zlen)*b_coef[2*k]*cos(2.0*pi*k*zz/zlen);
            bzpp = bzpp - pow(2.0*pi*k*zz/zlen,2)*b_coef[2*k-1]*cos(2.0*pi*k*zz/zlen) - pow(2.0*pi*k*zz/zlen,2)*b_coef[2*k]*sin(2.0*pi*k*zz/zlen);
            bzppp = bzppp + pow(2.0*pi*k*zz/zlen,3)*b_coef[2*k-1]*sin(2.0*pi*k*zz/zlen) - pow(2.0*pi*k*zz/zlen,3)*b_coef[2*k]*cos(2.0*pi*k*zz/zlen);
        }
        bz = bz*scale;          // Scale fields and derivatives by user-provided scaling factor (from input file)
        bzp = bzp*scale;
        bzpp = bzpp*scale;
        bzppp = bzppp*scale;

        amrex::ParticleReal r2 = x*x + y*y;

        Ex = 0.0;      // Update E and B fields from Fourier coefficients and scaling factors
        Ey = 0.0;
        Ez = 0.0;
        Bx = -(bzp*x/2) + (bzppp*x*r2/16);
        By = -(bzp*y/2) + (bzppp*y*r2/16);
        Bz = bz - (bzpp*r2/4);

    }

};

#endif // WARPX_ACCELERATORLATTICE_LATTICEELEMENTS_SOLENOID_H_
