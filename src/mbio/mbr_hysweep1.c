/*--------------------------------------------------------------------
 *    The MB-system:	mbr_hysweep1.c	12/23/2011
 *	$Id: mbr_hysweep1.c 2308 2017-06-04 19:55:48Z caress $
 *
 *    Copyright (c) 2011-2017 by
 *    David W. Caress (caress@mbari.org)
 *      Monterey Bay Aquarium Research Institute
 *      Moss Landing, CA 95039
 *    and Dale N. Chayes (dale@ldeo.columbia.edu)
 *      Lamont-Doherty Earth Observatory
 *      Palisades, NY 10964
 *
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/*
 * mbr_hysweep1.c contains the functions for reading and writing
 * multibeam data in the HYSWEEP1 format.
 * These functions include:
 *   mbr_alm_hysweep1	- allocate read/write memory
 *   mbr_dem_hysweep1	- deallocate read/write memory
 *   mbr_rt_hysweep1	- read and translate data
 *   mbr_wt_hysweep1	- translate and write data
 *
 * Author:	D. W. Caress
 * Date:	December 23,2011
 *
 *
 */

/* standard include files */
#include <stdio.h>
#include <math.h>
#include <string.h>

/* mbio include files */
#include "mb_status.h"
#include "mb_format.h"
#include "mb_io.h"
#include "mb_define.h"
#include "mbsys_hysweep.h"

/* include for byte swapping */
#include "mb_swap.h"

/* local defines */

/* turn on debug statements here */
// #define MBR_HYSWEEP1_DEBUG 1
// #define MBR_HYSWEEP1_DEBUG2 1

/* essential function prototypes */
int mbr_register_hysweep1(int verbose, void *mbio_ptr, int *error);
int mbr_info_hysweep1(int verbose, int *system, int *beams_bath_max, int *beams_amp_max, int *pixels_ss_max, char *format_name,
                      char *system_name, char *format_description, int *numfile, int *filetype, int *variable_beams,
                      int *traveltime, int *beam_flagging, int *platform_source, int *nav_source, int *sensordepth_source,
                      int *heading_source, int *attitude_source, int *svp_source, double *beamwidth_xtrack,
                      double *beamwidth_ltrack, int *error);
int mbr_alm_hysweep1(int verbose, void *mbio_ptr, int *error);
int mbr_dem_hysweep1(int verbose, void *mbio_ptr, int *error);
int mbr_rt_hysweep1(int verbose, void *mbio_ptr, void *store_ptr, int *error);
int mbr_wt_hysweep1(int verbose, void *mbio_ptr, void *store_ptr, int *error);

int mbr_hysweep1_rd_data(int verbose, void *mbio_ptr, void *store_ptr, int *error);
int mbr_hysweep1_rd_line(int verbose, FILE *mbfp, char *line, int *error);
int mbr_hysweep1_wr_data(int verbose, void *mbio_ptr, void *store_ptr, int *error);

static char rcs_id[] = "$Id: mbr_hysweep1.c 2308 2017-06-04 19:55:48Z caress $";

/*--------------------------------------------------------------------*/
int mbr_register_hysweep1(int verbose, void *mbio_ptr, int *error) {
	char *function_name = "mbr_register_hysweep1";
	int status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;

	/* print input debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", function_name);
		fprintf(stderr, "dbg2  Revision id: %s\n", rcs_id);
		fprintf(stderr, "dbg2  Input arguments:\n");
		fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
	}

	/* get mb_io_ptr */
	mb_io_ptr = (struct mb_io_struct *)mbio_ptr;

	/* set format info parameters */
	status = mbr_info_hysweep1(
	    verbose, &mb_io_ptr->system, &mb_io_ptr->beams_bath_max, &mb_io_ptr->beams_amp_max, &mb_io_ptr->pixels_ss_max,
	    mb_io_ptr->format_name, mb_io_ptr->system_name, mb_io_ptr->format_description, &mb_io_ptr->numfile, &mb_io_ptr->filetype,
	    &mb_io_ptr->variable_beams, &mb_io_ptr->traveltime, &mb_io_ptr->beam_flagging, &mb_io_ptr->platform_source,
	    &mb_io_ptr->nav_source, &mb_io_ptr->sensordepth_source, &mb_io_ptr->heading_source, &mb_io_ptr->attitude_source,
	    &mb_io_ptr->svp_source, &mb_io_ptr->beamwidth_xtrack, &mb_io_ptr->beamwidth_ltrack, error);

	/* set format and system specific function pointers */
	mb_io_ptr->mb_io_format_alloc = &mbr_alm_hysweep1;
	mb_io_ptr->mb_io_format_free = &mbr_dem_hysweep1;
	mb_io_ptr->mb_io_store_alloc = &mbsys_hysweep_alloc;
	mb_io_ptr->mb_io_store_free = &mbsys_hysweep_deall;
	mb_io_ptr->mb_io_read_ping = &mbr_rt_hysweep1;
	mb_io_ptr->mb_io_write_ping = &mbr_wt_hysweep1;
	mb_io_ptr->mb_io_dimensions = &mbsys_hysweep_dimensions;
	mb_io_ptr->mb_io_pingnumber = &mbsys_hysweep_pingnumber;
	mb_io_ptr->mb_io_extract_platform = &mbsys_hysweep_extract_platform;
	mb_io_ptr->mb_io_extract = &mbsys_hysweep_extract;
	mb_io_ptr->mb_io_insert = &mbsys_hysweep_insert;
	mb_io_ptr->mb_io_extract_nav = &mbsys_hysweep_extract_nav;
	mb_io_ptr->mb_io_extract_nnav = NULL;
	mb_io_ptr->mb_io_insert_nav = &mbsys_hysweep_insert_nav;
	mb_io_ptr->mb_io_extract_altitude = &mbsys_hysweep_extract_altitude;
	mb_io_ptr->mb_io_insert_altitude = NULL;
	mb_io_ptr->mb_io_extract_svp = NULL;
	mb_io_ptr->mb_io_insert_svp = NULL;
	mb_io_ptr->mb_io_ttimes = &mbsys_hysweep_ttimes;
	mb_io_ptr->mb_io_detects = &mbsys_hysweep_detects;
	mb_io_ptr->mb_io_gains = &mbsys_hysweep_gains;
	mb_io_ptr->mb_io_copyrecord = &mbsys_hysweep_copy;
	mb_io_ptr->mb_io_extract_rawss = NULL;
	mb_io_ptr->mb_io_insert_rawss = NULL;
	mb_io_ptr->mb_io_extract_segytraceheader = NULL;
	mb_io_ptr->mb_io_extract_segy = NULL;
	mb_io_ptr->mb_io_insert_segy = NULL;
	mb_io_ptr->mb_io_ctd = NULL;
	mb_io_ptr->mb_io_ancilliarysensor = NULL;

	/* print output debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", function_name);
		fprintf(stderr, "dbg2  Return values:\n");
		fprintf(stderr, "dbg2       system:             %d\n", mb_io_ptr->system);
		fprintf(stderr, "dbg2       beams_bath_max:     %d\n", mb_io_ptr->beams_bath_max);
		fprintf(stderr, "dbg2       beams_amp_max:      %d\n", mb_io_ptr->beams_amp_max);
		fprintf(stderr, "dbg2       pixels_ss_max:      %d\n", mb_io_ptr->pixels_ss_max);
		fprintf(stderr, "dbg2       format_name:        %s\n", mb_io_ptr->format_name);
		fprintf(stderr, "dbg2       system_name:        %s\n", mb_io_ptr->system_name);
		fprintf(stderr, "dbg2       format_description: %s\n", mb_io_ptr->format_description);
		fprintf(stderr, "dbg2       numfile:            %d\n", mb_io_ptr->numfile);
		fprintf(stderr, "dbg2       filetype:           %d\n", mb_io_ptr->filetype);
		fprintf(stderr, "dbg2       variable_beams:     %d\n", mb_io_ptr->variable_beams);
		fprintf(stderr, "dbg2       traveltime:         %d\n", mb_io_ptr->traveltime);
		fprintf(stderr, "dbg2       beam_flagging:      %d\n", mb_io_ptr->beam_flagging);
		fprintf(stderr, "dbg2       platform_source:    %d\n", mb_io_ptr->platform_source);
		fprintf(stderr, "dbg2       nav_source:         %d\n", mb_io_ptr->nav_source);
		fprintf(stderr, "dbg2       sensordepth_source: %d\n", mb_io_ptr->nav_source);
		fprintf(stderr, "dbg2       heading_source:     %d\n", mb_io_ptr->heading_source);
		fprintf(stderr, "dbg2       attitude_source:    %d\n", mb_io_ptr->attitude_source);
		fprintf(stderr, "dbg2       svp_source:         %d\n", mb_io_ptr->svp_source);
		fprintf(stderr, "dbg2       beamwidth_xtrack:   %f\n", mb_io_ptr->beamwidth_xtrack);
		fprintf(stderr, "dbg2       beamwidth_ltrack:   %f\n", mb_io_ptr->beamwidth_ltrack);
		fprintf(stderr, "dbg2       format_alloc:       %p\n", (void *)mb_io_ptr->mb_io_format_alloc);
		fprintf(stderr, "dbg2       format_free:        %p\n", (void *)mb_io_ptr->mb_io_format_free);
		fprintf(stderr, "dbg2       store_alloc:        %p\n", (void *)mb_io_ptr->mb_io_store_alloc);
		fprintf(stderr, "dbg2       store_free:         %p\n", (void *)mb_io_ptr->mb_io_store_free);
		fprintf(stderr, "dbg2       read_ping:          %p\n", (void *)mb_io_ptr->mb_io_read_ping);
		fprintf(stderr, "dbg2       write_ping:         %p\n", (void *)mb_io_ptr->mb_io_write_ping);
		fprintf(stderr, "dbg2       extract:            %p\n", (void *)mb_io_ptr->mb_io_extract);
		fprintf(stderr, "dbg2       insert:             %p\n", (void *)mb_io_ptr->mb_io_insert);
		fprintf(stderr, "dbg2       extract_nav:        %p\n", (void *)mb_io_ptr->mb_io_extract_nav);
		fprintf(stderr, "dbg2       insert_nav:         %p\n", (void *)mb_io_ptr->mb_io_insert_nav);
		fprintf(stderr, "dbg2       extract_altitude:   %p\n", (void *)mb_io_ptr->mb_io_extract_altitude);
		fprintf(stderr, "dbg2       insert_altitude:    %p\n", (void *)mb_io_ptr->mb_io_insert_altitude);
		fprintf(stderr, "dbg2       extract_svp:        %p\n", (void *)mb_io_ptr->mb_io_extract_svp);
		fprintf(stderr, "dbg2       insert_svp:         %p\n", (void *)mb_io_ptr->mb_io_insert_svp);
		fprintf(stderr, "dbg2       ttimes:             %p\n", (void *)mb_io_ptr->mb_io_ttimes);
		fprintf(stderr, "dbg2       detects:            %p\n", (void *)mb_io_ptr->mb_io_detects);
		fprintf(stderr, "dbg2       extract_rawss:      %p\n", (void *)mb_io_ptr->mb_io_extract_rawss);
		fprintf(stderr, "dbg2       insert_rawss:       %p\n", (void *)mb_io_ptr->mb_io_insert_rawss);
		fprintf(stderr, "dbg2       extract_segytraceheader: %p\n", (void *)mb_io_ptr->mb_io_extract_segytraceheader);
		fprintf(stderr, "dbg2       extract_segy:       %p\n", (void *)mb_io_ptr->mb_io_extract_segy);
		fprintf(stderr, "dbg2       insert_segy:        %p\n", (void *)mb_io_ptr->mb_io_insert_segy);
		fprintf(stderr, "dbg2       copyrecord:         %p\n", (void *)mb_io_ptr->mb_io_copyrecord);
		fprintf(stderr, "dbg2       error:              %d\n", *error);
		fprintf(stderr, "dbg2  Return status:\n");
		fprintf(stderr, "dbg2       status:         %d\n", status);
	}

	/* return status */
	return (status);
}

/*--------------------------------------------------------------------*/
int mbr_info_hysweep1(int verbose, int *system, int *beams_bath_max, int *beams_amp_max, int *pixels_ss_max, char *format_name,
                      char *system_name, char *format_description, int *numfile, int *filetype, int *variable_beams,
                      int *traveltime, int *beam_flagging, int *platform_source, int *nav_source, int *sensordepth_source,
                      int *heading_source, int *attitude_source, int *svp_source, double *beamwidth_xtrack,
                      double *beamwidth_ltrack, int *error) {
	char *function_name = "mbr_info_hysweep1";
	int status = MB_SUCCESS;

	/* print input debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", function_name);
		fprintf(stderr, "dbg2  Revision id: %s\n", rcs_id);
		fprintf(stderr, "dbg2  Input arguments:\n");
		fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
	}

	/* set format info parameters */
	status = MB_SUCCESS;
	*error = MB_ERROR_NO_ERROR;
	*system = MB_SYS_HYSWEEP;
	*beams_bath_max = 512;
	*beams_amp_max = 512;
	*pixels_ss_max = MBSYS_HYSWEEP_MSS_NUM_PIXELS;
	strncpy(format_name, "HYSWEEP1", MB_NAME_LENGTH);
	strncpy(system_name, "HYSWEEP", MB_NAME_LENGTH);
	strncpy(format_description,
	        "Format name:          MBF_HYSWEEP1\nInformal Description: HYSWEEP multibeam data format\nAttributes:           Many "
	        "multibeam sonars, \n                      bathymetry, amplitude\n                      variable beams, ascii, "
	        "HYPACK.\n",
	        MB_DESCRIPTION_LENGTH);
	*numfile = 1;
	*filetype = MB_FILETYPE_NORMAL;
	*variable_beams = MB_YES;
	*traveltime = MB_YES;
	*beam_flagging = MB_YES;
	*platform_source = MB_DATA_HEADER;
	*nav_source = MB_DATA_DATA;
	*sensordepth_source = MB_DATA_DATA;
	*heading_source = MB_DATA_DATA;
	*attitude_source = MB_DATA_DATA;
	*svp_source = MB_DATA_VELOCITY_PROFILE;
	*beamwidth_xtrack = 1.0;
	*beamwidth_ltrack = 1.0;

	/* print output debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", function_name);
		fprintf(stderr, "dbg2  Return values:\n");
		fprintf(stderr, "dbg2       system:             %d\n", *system);
		fprintf(stderr, "dbg2       beams_bath_max:     %d\n", *beams_bath_max);
		fprintf(stderr, "dbg2       beams_amp_max:      %d\n", *beams_amp_max);
		fprintf(stderr, "dbg2       pixels_ss_max:      %d\n", *pixels_ss_max);
		fprintf(stderr, "dbg2       format_name:        %s\n", format_name);
		fprintf(stderr, "dbg2       system_name:        %s\n", system_name);
		fprintf(stderr, "dbg2       format_description: %s\n", format_description);
		fprintf(stderr, "dbg2       numfile:            %d\n", *numfile);
		fprintf(stderr, "dbg2       filetype:           %d\n", *filetype);
		fprintf(stderr, "dbg2       variable_beams:     %d\n", *variable_beams);
		fprintf(stderr, "dbg2       traveltime:         %d\n", *traveltime);
		fprintf(stderr, "dbg2       beam_flagging:      %d\n", *beam_flagging);
		fprintf(stderr, "dbg2       platform_source:    %d\n", *platform_source);
		fprintf(stderr, "dbg2       nav_source:         %d\n", *nav_source);
		fprintf(stderr, "dbg2       sensordepth_source: %d\n", *sensordepth_source);
		fprintf(stderr, "dbg2       heading_source:     %d\n", *heading_source);
		fprintf(stderr, "dbg2       attitude_source:      %d\n", *attitude_source);
		fprintf(stderr, "dbg2       svp_source:         %d\n", *svp_source);
		fprintf(stderr, "dbg2       beamwidth_xtrack:   %f\n", *beamwidth_xtrack);
		fprintf(stderr, "dbg2       beamwidth_ltrack:   %f\n", *beamwidth_ltrack);
		fprintf(stderr, "dbg2       error:              %d\n", *error);
		fprintf(stderr, "dbg2  Return status:\n");
		fprintf(stderr, "dbg2       status:         %d\n", status);
	}

	/* return status */
	return (status);
}
/*--------------------------------------------------------------------*/
int mbr_alm_hysweep1(int verbose, void *mbio_ptr, int *error) {
	char *function_name = "mbr_alm_hysweep1";
	int status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;
	int *file_header_read;
	int *file_header_written;
	int *line_saved;
	int *RMB_read;
	double *pixel_size;
	double *swath_width;

	/* print input debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", function_name);
		fprintf(stderr, "dbg2  Revision id: %s\n", rcs_id);
		fprintf(stderr, "dbg2  Input arguments:\n");
		fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
		fprintf(stderr, "dbg2       mbio_ptr:   %p\n", (void *)mbio_ptr);
	}

	/* get pointer to mbio descriptor */
	mb_io_ptr = (struct mb_io_struct *)mbio_ptr;

	/* set initial status */
	status = MB_SUCCESS;

	/* allocate memory for data structure */
	mb_io_ptr->structure_size = 0;
	mb_io_ptr->data_structure_size = 0;
	status = mbsys_hysweep_alloc(verbose, mbio_ptr, &mb_io_ptr->store_data, error);

	/* set saved values */
	file_header_read = (int *)&mb_io_ptr->save1;
	file_header_written = (int *)&mb_io_ptr->save2;
	line_saved = (int *)&mb_io_ptr->save3;
	RMB_read = (int *)&mb_io_ptr->save4;
	*file_header_read = MB_NO;
	*file_header_written = MB_NO;
	*line_saved = MB_NO;
	*RMB_read = MB_NO;
	pixel_size = (double *)&mb_io_ptr->saved1;
	swath_width = (double *)&mb_io_ptr->saved2;

	*pixel_size = 0.0;
	*swath_width = 0.0;

	/* print output debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", function_name);
		fprintf(stderr, "dbg2  Return values:\n");
		fprintf(stderr, "dbg2       error:      %d\n", *error);
		fprintf(stderr, "dbg2  Return status:\n");
		fprintf(stderr, "dbg2       status:  %d\n", status);
	}

	/* return status */
	return (status);
}
/*--------------------------------------------------------------------*/
int mbr_dem_hysweep1(int verbose, void *mbio_ptr, int *error) {
	char *function_name = "mbr_dem_hysweep1";
	int status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;

	/* print input debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", function_name);
		fprintf(stderr, "dbg2  Revision id: %s\n", rcs_id);
		fprintf(stderr, "dbg2  Input arguments:\n");
		fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
		fprintf(stderr, "dbg2       mbio_ptr:   %p\n", (void *)mbio_ptr);
	}

	/* get pointers to mbio descriptor */
	mb_io_ptr = (struct mb_io_struct *)mbio_ptr;

	/* deallocate memory for data descriptor */
	status = mbsys_hysweep_deall(verbose, mbio_ptr, &mb_io_ptr->store_data, error);

	/* print output debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", function_name);
		fprintf(stderr, "dbg2  Return values:\n");
		fprintf(stderr, "dbg2       error:      %d\n", *error);
		fprintf(stderr, "dbg2  Return status:\n");
		fprintf(stderr, "dbg2       status:  %d\n", status);
	}

	/* return status */
	return (status);
}
/*--------------------------------------------------------------------*/
int mbr_rt_hysweep1(int verbose, void *mbio_ptr, void *store_ptr, int *error) {
	char *function_name = "mbr_rt_hysweep1";
	int status = MB_SUCCESS;
	int interp_status;
	int interp_error = MB_ERROR_NO_ERROR;
	struct mb_io_struct *mb_io_ptr;
	struct mbsys_hysweep_struct *store;
	struct mbsys_hysweep_device_struct *device;
	double navlon, navlat;
	double roll, speed;
	double alpha, beta, theta, phi;
	double rr, xx, zz;
	double *pixel_size, *swath_width;
	int i;

	/* print input debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", function_name);
		fprintf(stderr, "dbg2  Revision id: %s\n", rcs_id);
		fprintf(stderr, "dbg2  Input arguments:\n");
		fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
		fprintf(stderr, "dbg2       mbio_ptr:   %p\n", (void *)mbio_ptr);
		fprintf(stderr, "dbg2       store_ptr:  %p\n", (void *)store_ptr);
	}

	/* get pointers to mbio descriptor */
	mb_io_ptr = (struct mb_io_struct *)mbio_ptr;

	/* read next data from file */
	status = mbr_hysweep1_rd_data(verbose, mbio_ptr, store_ptr, error);

	/* get pointers to data structures */
	store = (struct mbsys_hysweep_struct *)store_ptr;
	pixel_size = (double *)&mb_io_ptr->saved1;
	swath_width = (double *)&mb_io_ptr->saved2;

	/* save position if primary data */
	if (status == MB_SUCCESS && (store->kind == MB_DATA_NAV || store->kind == MB_DATA_NAV1 || store->kind == MB_DATA_NAV2)) {
		/* check device for being enabled */
		device = (struct mbsys_hysweep_device_struct *)&(store->devices[store->POS_device_number]);
		if (device->DV2_enabled == MB_YES) {
			/* add latest fix */
			if (mb_io_ptr->projection_initialized == MB_YES) {
				mb_proj_inverse(verbose, mb_io_ptr->pjptr, store->POS_x, store->POS_y, &navlon, &navlat, error);
			}
			else {
				navlon = store->POS_x;
				navlat = store->POS_y;
			}
			mb_navint_add(verbose, mbio_ptr, store->time_d, navlon, navlat, error);
			/* fprintf(stderr,"POS %4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%6.6d   %f %f   %f %f\n",
			store->time_i[0],store->time_i[1],store->time_i[2],store->time_i[3],store->time_i[4],store->time_i[5],store->time_i[6],
			store->POS_x,store->POS_y,navlon,navlat);*/
		}
	}

	/* save attitude if primary data */
	if (status == MB_SUCCESS && store->kind == MB_DATA_ATTITUDE) {
		/* check device for being enabled */
		device = (struct mbsys_hysweep_device_struct *)&(store->devices[store->HCP_device_number]);
		if (device->DV2_enabled == MB_YES) {
			/* add latest attitude */
			mb_attint_add(verbose, mbio_ptr, store->time_d, store->HCP_heave, -store->HCP_roll, store->HCP_pitch, error);
		}
	}

	/* save heading if primary data */
	if (status == MB_SUCCESS && store->kind == MB_DATA_HEADING) {
		/* check device for being enabled */
		device = (struct mbsys_hysweep_device_struct *)&(store->devices[store->GYR_device_number]);
		if (device->DV2_enabled == MB_YES) {
			/* add latest attitude */
			mb_hedint_add(verbose, mbio_ptr, store->time_d, store->GYR_heading, error);
		}
	}

	/* save sonardepth if primary data */
	if (status == MB_SUCCESS && store->kind == MB_DATA_SONARDEPTH) {
		/* check device for being enabled */
		device = (struct mbsys_hysweep_device_struct *)&(store->devices[store->DFT_device_number]);
		if (device->DV2_enabled == MB_YES) {
			/* add latest attitude */
			mb_depint_add(verbose, mbio_ptr, store->time_d, store->DFT_draft, error);
		}
	}

	/* save altitude if primary data */
	if (status == MB_SUCCESS && store->kind == MB_DATA_ALTITUDE) {
		/* check device for being enabled */
		device = (struct mbsys_hysweep_device_struct *)&(store->devices[store->EC1_device_number]);
		if (device->DV2_enabled == MB_YES) {
			/* add latest attitude */
			mb_altint_add(verbose, mbio_ptr, store->time_d, store->EC1_rawdepth, error);
		}
	}
#ifdef MBR_HYSWEEP1_DEBUG
	if (verbose > 0)
		fprintf(stderr, "Record returned: type:%d status:%d error:%d\n\n", store->kind, status, *error);
#endif

	/* if survey data then interpolate nav, heading, attitude, sonardepth onto ping times */
	if (status == MB_SUCCESS && store->kind == MB_DATA_DATA) {
		speed = 0.0;
		interp_status = mb_hedint_interp(verbose, mbio_ptr, store->time_d, &(store->RMBint_heading), &interp_error);
		interp_status = mb_depint_interp(verbose, mbio_ptr, store->time_d, &(store->RMBint_draft), &interp_error);

		/* ignore heading and sonar depth errors */
		interp_error = MB_ERROR_NO_ERROR;

		interp_status = mb_attint_interp(verbose, mbio_ptr, store->time_d, &(store->RMBint_heave), &(roll),
		                                 &(store->RMBint_pitch), &interp_error);
		store->RMBint_roll = -roll;
		interp_status = mb_navint_interp(verbose, mbio_ptr, store->time_d, store->RMBint_heading, speed, &(store->RMBint_lon),
		                                 &(store->RMBint_lat), &speed, &interp_error);
		if (interp_status == MB_SUCCESS) {
			if (mb_io_ptr->projection_initialized == MB_YES) {
				mb_proj_forward(verbose, mb_io_ptr->pjptr, store->RMBint_lon, store->RMBint_lat, &(store->RMBint_x),
				                &(store->RMBint_y), error);
			}
			else {
				store->RMBint_x = store->RMBint_lon;
				store->RMBint_y = store->RMBint_lat;
			}
		}
		else {
			store->RMBint_lon = 0.0;
			store->RMBint_lat = 0.0;
			store->RMBint_x = 0.0;
			store->RMBint_y = 0.0;
		}
		/* fprintf(stderr,"RMB %4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%6.6d   %f %f   %f %f\n",
		store->time_i[0],store->time_i[1],store->time_i[2],store->time_i[3],store->time_i[4],store->time_i[5],store->time_i[6],
		store->RMBint_x,store->RMBint_y,store->RMBint_lon,store->RMBint_lat);*/
	}

	/* if survey data then calculate angles and bathymetry as necessary */
	if (status == MB_SUCCESS && store->kind == MB_DATA_DATA) {
		/* get mapping sonar device pointer */
		device = (struct mbsys_hysweep_device_struct *)&(store->devices[store->RMB_device_number]);

		/* deal with case of multibeam sonar */
		if (store->RMB_beam_data_available & 0x0001) {
			/* handle data that starts with beam angles in roll and pitch coordinates */
			if (store->RMB_sonar_type == 1 || store->RMB_sonar_type == 2) {
				/* get beam roll angles if necessary */
				if (!(store->RMB_beam_data_available & 0x0080)) {
					for (i = 0; i < store->RMB_num_beams; i++) {
						store->RMB_sounding_rollangles[i] = device->MBI_first_beam_angle + i * device->MBI_angle_increment;
					}
					store->RMB_beam_data_available = store->RMB_beam_data_available | 0x0080;
				}

				/* get beam pitch angles if necessary */
				if (!(store->RMB_beam_data_available & 0x0040)) {
					for (i = 0; i < store->RMB_num_beams; i++) {
						store->RMB_sounding_pitchangles[i] = 0.0;
					}
					store->RMB_beam_data_available = store->RMB_beam_data_available | 0x0040;
				}
			}

			/* get beam takeoff and azimuthal angles if necessary */
			if (!(store->RMB_beam_data_available & 0x0100) || !(store->RMB_beam_data_available & 0x0200)) {
				for (i = 0; i < store->RMB_num_beams; i++) {
					alpha = store->RMB_sounding_pitchangles[i];
					beta = 90.0 + store->RMB_sounding_rollangles[i];

					/* correct alpha for pitch if necessary */
					if (!(device->MBI_sonar_flags & 0x0002))
						alpha += store->RMBint_pitch;

					/* correct beta for roll if necessary */
					if (!(device->MBI_sonar_flags & 0x0001))
						beta += store->RMBint_roll;

					mb_rollpitch_to_takeoff(verbose, alpha, beta, &theta, &phi, error);
					store->RMB_sounding_takeoffangles[i] = theta;
					store->RMB_sounding_azimuthalangles[i] = 90.0 - phi;
				}
				store->RMB_beam_data_available = store->RMB_beam_data_available | 0x0300;
			}

			/* get beam bathymetry if necessary */
			if (!(store->RMB_beam_data_available & 0x0008) || !(store->RMB_beam_data_available & 0x0010) ||
			    !(store->RMB_beam_data_available & 0x0020)) {
				for (i = 0; i < store->RMB_num_beams; i++) {
					rr = store->RMB_beam_ranges[i];
					theta = store->RMB_sounding_takeoffangles[i];
					phi = 90.0 - store->RMB_sounding_azimuthalangles[i];
					xx = rr * sin(DTR * theta);
					zz = rr * cos(DTR * theta);
					store->RMB_sounding_across[i] = xx * cos(DTR * phi);
					store->RMB_sounding_along[i] = xx * sin(DTR * phi);
					store->RMB_sounding_depths[i] = zz + store->RMBint_draft + store->RMBint_heave;
				}
				store->RMB_beam_data_available = store->RMB_beam_data_available | 0x0038;
			}

			/* get beam flags if necessary */
			if (!(store->RMB_beam_data_available & 0x2000)) {
				for (i = 0; i < store->RMB_num_beams; i++) {
					store->RMB_sounding_flags[i] = MB_FLAG_NONE;
				}
				store->RMB_beam_data_available = store->RMB_beam_data_available | 0x2000;

				/* incorporate quality values */
				if ((store->RMB_beam_data_available & 0x1000) && strncmp(device->DEV_device_name, "Reson Seabat 8", 14) == 0) {
					for (i = 0; i < store->RMB_num_beams; i++) {
						if (store->RMB_sounding_quality[i] < 2)
							store->RMB_sounding_flags[i] = MB_FLAG_FLAG + MB_FLAG_SONAR;
					}
				}

				/* check for null ranges */
				if ((store->RMB_beam_data_available & 0x0001)) {
					for (i = 0; i < store->RMB_num_beams; i++) {
						if (store->RMB_beam_ranges[i] <= 0.0)
							store->RMB_sounding_flags[i] = MB_FLAG_FLAG + MB_FLAG_SONAR;
					}
				}
			}
		}

		/* deal with case of multiple transducer sonar */
		if (store->RMB_beam_data_available & 0x0002) {
			/* get beam roll angles if necessary */
			if (!(store->RMB_beam_data_available & 0x0080)) {
				for (i = 0; i < store->RMB_num_beams; i++) {
					store->RMB_sounding_rollangles[i] = 0.0;
				}
				store->RMB_beam_data_available = store->RMB_beam_data_available | 0x0080;
			}

			/* correct beam roll angles for roll if necessary */
			if (!(device->MBI_sonar_flags & 0x0001)) {
				for (i = 0; i < store->RMB_num_beams; i++) {
					store->RMB_sounding_rollangles[i] -= store->RMBint_roll;
				}
			}

			/* get beam pitch angles if necessary */
			if (!(store->RMB_beam_data_available & 0x0040)) {
				if (!(device->MBI_sonar_flags & 0x0002)) {
					for (i = 0; i < store->RMB_num_beams; i++) {
						store->RMB_sounding_pitchangles[i] = store->RMBint_pitch;
					}
				}
				else {
					for (i = 0; i < store->RMB_num_beams; i++) {
						store->RMB_sounding_pitchangles[i] = 0.0;
					}
				}
				store->RMB_beam_data_available = store->RMB_beam_data_available | 0x0040;
			}

			/* get beam takeoff and azimuthal angles if necessary */
			if (!(store->RMB_beam_data_available & 0x0100) || !(store->RMB_beam_data_available & 0x0200)) {
				for (i = 0; i < store->RMB_num_beams; i++) {
					alpha = store->RMB_sounding_pitchangles[i];
					beta = 90.0 - store->RMB_sounding_rollangles[i];
					mb_rollpitch_to_takeoff(verbose, alpha, beta, &theta, &phi, error);
					store->RMB_sounding_takeoffangles[i] = theta;
					store->RMB_sounding_azimuthalangles[i] = 90.0 - phi;
				}
				store->RMB_beam_data_available = store->RMB_beam_data_available | 0x0300;
			}

			/* get beam bathymetry if necessary */
			if (!(store->RMB_beam_data_available & 0x0004) || !(store->RMB_beam_data_available & 0x0008) ||
			    !(store->RMB_beam_data_available & 0x0010) || !(store->RMB_beam_data_available & 0x0020)) {
				for (i = 0; i < store->RMB_num_beams; i++) {
					rr = store->RMB_multi_ranges[i];
					theta = store->RMB_sounding_takeoffangles[i];
					phi = 90.0 - store->RMB_sounding_azimuthalangles[i];
					xx = rr * sin(DTR * theta);
					zz = rr * cos(DTR * theta);
					store->RMB_sounding_across[i] = xx * cos(DTR * phi);
					store->RMB_sounding_along[i] = xx * sin(DTR * phi);
					store->RMB_sounding_depths[i] = zz + store->RMBint_draft + store->RMBint_heave;
				}
				store->RMB_beam_data_available = store->RMB_beam_data_available | 0x003C;
			}

			/* get beam flags if necessary */
			if (!(store->RMB_beam_data_available & 0x2000)) {
				for (i = 0; i < store->RMB_num_beams; i++) {
					store->RMB_sounding_flags[i] = MB_FLAG_NONE;
				}
				store->RMB_beam_data_available = store->RMB_beam_data_available | 0x2000;
			}
		}

		/* generate processed sidescan if needed */
		if (store->MSS_ping_number != store->RSS_ping_number &&
		    (store->RSS_ping_number == store->RMB_ping_number || 10 * store->RSS_ping_number == store->RMB_ping_number)) {
			status = mbsys_hysweep_makess(verbose, mbio_ptr, store_ptr, MB_NO, pixel_size, MB_NO, swath_width, 5, error);
		}

		/* print debug statements */
		if (verbose >= 4) {
			fprintf(stderr, "\ndbg4  Multibeam bathymetry calculated by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       RMB_device_number:                 %d\n", store->RMB_device_number);
			fprintf(stderr, "dbg4       RMB_time:                          %f\n", store->RMB_time);
			fprintf(stderr, "dbg4       RMB_sonar_type:                    %x\n", store->RMB_sonar_type);
			fprintf(stderr, "dbg4       RMB_sonar_flags:                   %x\n", store->RMB_sonar_flags);
			fprintf(stderr, "dbg4       RMB_beam_data_available:           %x\n", store->RMB_beam_data_available);
			fprintf(stderr, "dbg4       RMB_num_beams:                     %d\n", store->RMB_num_beams);
			fprintf(stderr, "dbg4       RMB_num_beams_alloc:               %d\n", store->RMB_num_beams_alloc);
			fprintf(stderr, "dbg4       RMB_sound_velocity:                %f\n", store->RMB_sound_velocity);
			fprintf(stderr, "dbg4       RMB_ping_number:                   %d\n", store->RMB_ping_number);
			for (i = 0; i < store->RMB_num_beams; i++) {
				fprintf(stderr, "dbg4       beam:%4d", i);

				if (store->RMB_beam_data_available & 0x0001)
					fprintf(stderr, " mbrng:%f", store->RMB_beam_ranges[i]);

				if (store->RMB_beam_data_available & 0x0002)
					fprintf(stderr, " mtrng:%f", store->RMB_multi_ranges[i]);

				if (store->RMB_beam_data_available & 0x0004)
					fprintf(stderr, " est:%f", store->RMB_sounding_eastings[i]);

				if (store->RMB_beam_data_available & 0x0004)
					fprintf(stderr, " nor:%f", store->RMB_sounding_northings[i]);

				if (store->RMB_beam_data_available & 0x0008)
					fprintf(stderr, " dep:%f", store->RMB_sounding_depths[i]);

				if (store->RMB_beam_data_available & 0x0010)
					fprintf(stderr, " ltr:%f", store->RMB_sounding_along[i]);

				if (store->RMB_beam_data_available & 0x0020)
					fprintf(stderr, " atr:%f", store->RMB_sounding_across[i]);

				if (store->RMB_beam_data_available & 0x0040)
					fprintf(stderr, " pth:%f", store->RMB_sounding_pitchangles[i]);

				if (store->RMB_beam_data_available & 0x0080)
					fprintf(stderr, " rll:%f", store->RMB_sounding_rollangles[i]);

				if (store->RMB_beam_data_available & 0x0100)
					fprintf(stderr, " toa:%f", store->RMB_sounding_takeoffangles[i]);

				if (store->RMB_beam_data_available & 0x0200)
					fprintf(stderr, " azi:%f", store->RMB_sounding_azimuthalangles[i]);

				if (store->RMB_beam_data_available & 0x0400)
					fprintf(stderr, " tim:%d", store->RMB_sounding_timedelays[i]);

				if (store->RMB_beam_data_available & 0x0800)
					fprintf(stderr, " int:%d", store->RMB_sounding_intensities[i]);

				if (store->RMB_beam_data_available & 0x1000)
					fprintf(stderr, " qua:%d", store->RMB_sounding_quality[i]);

				if (store->RMB_beam_data_available & 0x2000)
					fprintf(stderr, " flg:%d", store->RMB_sounding_flags[i]);

				fprintf(stderr, "\n");
			}
		}
	}

	/* set error and kind in mb_io_ptr */
	mb_io_ptr->new_error = *error;
	mb_io_ptr->new_kind = store->kind;

	/* print output debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", function_name);
		fprintf(stderr, "dbg2  Return values:\n");
		fprintf(stderr, "dbg2       error:      %d\n", *error);
		fprintf(stderr, "dbg2  Return status:\n");
		fprintf(stderr, "dbg2       status:  %d\n", status);
	}

	/* return status */
	return (status);
}
/*--------------------------------------------------------------------*/
int mbr_wt_hysweep1(int verbose, void *mbio_ptr, void *store_ptr, int *error) {
	char *function_name = "mbr_wt_hysweep1";
	int status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;
	struct mbsys_hysweep_struct *store;

	/* print input debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", function_name);
		fprintf(stderr, "dbg2  Revision id: %s\n", rcs_id);
		fprintf(stderr, "dbg2  Input arguments:\n");
		fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
		fprintf(stderr, "dbg2       mbio_ptr:   %p\n", (void *)mbio_ptr);
		fprintf(stderr, "dbg2       store_ptr:  %p\n", (void *)store_ptr);
	}

	/* get pointer to mbio descriptor */
	mb_io_ptr = (struct mb_io_struct *)mbio_ptr;

	/* get pointer to raw data structure */
	store = (struct mbsys_hysweep_struct *)store_ptr;

	/* write next data to file */
	status = mbr_hysweep1_wr_data(verbose, mbio_ptr, store_ptr, error);

	/* print output debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", function_name);
		fprintf(stderr, "dbg2  Return values:\n");
		fprintf(stderr, "dbg2       error:      %d\n", *error);
		fprintf(stderr, "dbg2  Return status:\n");
		fprintf(stderr, "dbg2       status:  %d\n", status);
	}

	/* return status */
	return (status);
}
/*--------------------------------------------------------------------*/
int mbr_hysweep1_rd_data(int verbose, void *mbio_ptr, void *store_ptr, int *error) {
	char *function_name = "mbr_hysweep1_rd_data";
	int status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;
	struct mbsys_hysweep_struct *store;
	struct mbsys_hysweep_device_struct *device;
	struct mbsys_hysweep_device_offset_struct *offset;
	int *file_header_read;
	int *file_header_written;
	int *line_saved;
	int *RMB_read;
	int done;
	char *line;
	int nscan, nread;
	char *token;
	int DEV_device_number;
	int DEV_device_capability;
	mb_path DEV_device_name;
	int DV2_device_number;
	int DV2_device_capability;
	int DV2_towfish;
	int DV2_enabled;
	int HVF_device_number;
	int MBI_device_number;
	int MBI_sonar_type;
	int MBI_sonar_flags;
	int MBI_beam_data_available;
	int MBI_num_beams_1;
	int MBI_num_beams_2;
	double MBI_first_beam_angle;
	double MBI_angle_increment;
	int OF2_device_number;
	int OF2_offset_type;
	double OF2_offset_starboard;
	double OF2_offset_forward;
	double OF2_offset_vertical;
	double OF2_offset_yaw;
	double OF2_offset_roll;
	double OF2_offset_pitch;
	double OF2_offset_time;
	int SSI_device_number;
	int SSI_sonar_flags;
	int SSI_port_num_samples;
	int SSI_starboard_num_samples;
	int tmpRMB_device_number;
	double tmpRMB_time;
	int tmpRMB_sonar_type;
	int tmpRMB_sonar_flags;
	int tmpRMB_beam_data_available;
	int tmpRMB_num_beams;
	double tmpRMB_sound_velocity;
	int tmpRMB_ping_number;
	int SNRok, RSSok;
	int len;
	int i;

	/* print input debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", function_name);
		fprintf(stderr, "dbg2  Revision id: %s\n", rcs_id);
		fprintf(stderr, "dbg2  Input arguments:\n");
		fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
		fprintf(stderr, "dbg2       mbio_ptr:   %p\n", (void *)mbio_ptr);
		fprintf(stderr, "dbg2       store_ptr:  %p\n", (void *)store_ptr);
	}

	/* get pointer to mbio descriptor */
	mb_io_ptr = (struct mb_io_struct *)mbio_ptr;

	/* get pointer to raw data structure */
	store = (struct mbsys_hysweep_struct *)store_ptr;
	line = store->readline;

	/* get saved values */
	file_header_read = (int *)&mb_io_ptr->save1;
	file_header_written = (int *)&mb_io_ptr->save2;
	line_saved = (int *)&mb_io_ptr->save3;
	RMB_read = (int *)&mb_io_ptr->save4;

	/* set file position */
	mb_io_ptr->file_pos = mb_io_ptr->file_bytes;

	/* loop over reading data until a record is ready for return */
	done = MB_NO;
	while (*error == MB_ERROR_NO_ERROR && done == MB_NO) {
		/* if (*line_saved == MB_YES)
		fprintf(stderr,"SAVED:"); */
		/* read the next line */
		if (*line_saved == MB_NO)
			status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);
		else
			*line_saved = MB_NO;
		/* fprintf(stderr,"line:%s",line); */

		/* now make sense of the line */
		if (status == MB_SUCCESS) {
			/* check if a new record has been encountered before the last one
			    has been processed */
			if (*RMB_read == MB_YES && strncmp(line, "RMB", 3) == 0) {
				/* check for erroneous RMB records associated with non-multibeam devices */
				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %x %x %x %d %lf %d", &(tmpRMB_device_number), &(tmpRMB_time),
				               &(tmpRMB_sonar_type), &(tmpRMB_sonar_flags), &(tmpRMB_beam_data_available), &(tmpRMB_num_beams),
				               &(tmpRMB_sound_velocity), &(tmpRMB_ping_number));
				if (nscan == 8 && store->devices[tmpRMB_device_number].DEV_device_capability >= 32768) {
					*line_saved = MB_YES;
					done = MB_YES;
					store->kind = MB_DATA_DATA;
					store->time_d = store->TND_survey_time_d + store->RMB_time;
					mb_get_date(verbose, store->time_d, store->time_i);
					*RMB_read = MB_NO;
				}
				else {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}
			}

			/* RMB multibeam data record */
			else if (strncmp(line, "RMB", 3) == 0) {
				/* fprintf(stderr,"Reading line because *RMB_read:%d\n",*RMB_read); */

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %x %x %x %d %lf %d", &(tmpRMB_device_number), &(tmpRMB_time),
				               &(tmpRMB_sonar_type), &(tmpRMB_sonar_flags), &(tmpRMB_beam_data_available), &(tmpRMB_num_beams),
				               &(tmpRMB_sound_velocity), &(tmpRMB_ping_number));
				if (nscan == 8 && store->devices[tmpRMB_device_number].DEV_device_capability >= 32768) {
					store->type = MBSYS_HYSWEEP_RECORDTYPE_RMB;
					store->RMB_device_number = tmpRMB_device_number;
					store->RMB_time = tmpRMB_time;
					store->RMB_sonar_type = tmpRMB_sonar_type;
					store->RMB_sonar_flags = tmpRMB_sonar_flags;
					store->RMB_beam_data_available = tmpRMB_beam_data_available;
					store->RMB_num_beams = tmpRMB_num_beams;
					store->RMB_sound_velocity = tmpRMB_sound_velocity;
					store->RMB_ping_number = tmpRMB_ping_number;
				}
				else {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* allocate space for beam data if required */
				if (status == MB_SUCCESS && store->RMB_num_beams > store->RMB_num_beams_alloc) {
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_beam_ranges), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_multi_ranges), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_sounding_eastings), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_sounding_northings), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_sounding_depths), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_sounding_across), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_sounding_along), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_sounding_pitchangles), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_sounding_rollangles), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_sounding_takeoffangles), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(double),
					                     (void **)&(store->RMB_sounding_azimuthalangles), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(int),
					                     (void **)&(store->RMB_sounding_timedelays), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(int),
					                     (void **)&(store->RMB_sounding_intensities), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(int),
					                     (void **)&(store->RMB_sounding_quality), error);
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RMB_num_beams * sizeof(int),
					                     (void **)&(store->RMB_sounding_flags), error);
				}

				/* read and parse RMB_beam_ranges if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0001) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_beam_ranges[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_multi_ranges if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0002) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_multi_ranges[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse   if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0004) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_sounding_eastings[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_northings if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0004) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_sounding_northings[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_depths if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0008) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_sounding_depths[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_along if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0010) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_sounding_along[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_across if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0020) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_sounding_across[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_pitchangles if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0040) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_sounding_pitchangles[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_rollangles if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0080) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_sounding_rollangles[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_takeoffangles if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0100) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_sounding_takeoffangles[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_azimuthalangles if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0200) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->RMB_sounding_azimuthalangles[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_timedelays if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0400) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%d", &(store->RMB_sounding_timedelays[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_intensities if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x0800) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%d", &(store->RMB_sounding_intensities[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_quality if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x1000) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%d", &(store->RMB_sounding_quality[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RMB_sounding_flags if included */
				if (status == MB_SUCCESS && store->RMB_beam_data_available & 0x2000) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RMB_num_beams && token != NULL; i++) {
							nscan = sscanf(token, "%d", &(store->RMB_sounding_flags[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RMB_num_beams) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  RMB data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       RMB_device_number:                 %d\n", store->RMB_device_number);
					fprintf(stderr, "dbg4       RMB_time:                          %f\n", store->RMB_time);
					fprintf(stderr, "dbg4       RMB_sonar_type:                    %x\n", store->RMB_sonar_type);
					fprintf(stderr, "dbg4       RMB_sonar_flags:                   %x\n", store->RMB_sonar_flags);
					fprintf(stderr, "dbg4       RMB_beam_data_available:           %x\n", store->RMB_beam_data_available);
					fprintf(stderr, "dbg4       RMB_num_beams:                     %d\n", store->RMB_num_beams);
					fprintf(stderr, "dbg4       RMB_num_beams_alloc:               %d\n", store->RMB_num_beams_alloc);
					fprintf(stderr, "dbg4       RMB_sound_velocity:                %f\n", store->RMB_sound_velocity);
					fprintf(stderr, "dbg4       RMB_ping_number:                   %d\n", store->RMB_ping_number);
					for (i = 0; i < store->RMB_num_beams; i++) {
						fprintf(stderr, "dbg4       beam:%4d", i);

						if (store->RMB_beam_data_available & 0x0001)
							fprintf(stderr, " %f", store->RMB_beam_ranges[i]);

						if (store->RMB_beam_data_available & 0x0002)
							fprintf(stderr, " %f", store->RMB_multi_ranges[i]);

						if (store->RMB_beam_data_available & 0x0004)
							fprintf(stderr, " %f", store->RMB_sounding_eastings[i]);

						if (store->RMB_beam_data_available & 0x0004)
							fprintf(stderr, " %f", store->RMB_sounding_northings[i]);

						if (store->RMB_beam_data_available & 0x0008)
							fprintf(stderr, " %f", store->RMB_sounding_depths[i]);

						if (store->RMB_beam_data_available & 0x0010)
							fprintf(stderr, " %f", store->RMB_sounding_along[i]);

						if (store->RMB_beam_data_available & 0x0020)
							fprintf(stderr, " %f", store->RMB_sounding_across[i]);

						if (store->RMB_beam_data_available & 0x0040)
							fprintf(stderr, " %f", store->RMB_sounding_pitchangles[i]);

						if (store->RMB_beam_data_available & 0x0080)
							fprintf(stderr, " %f", store->RMB_sounding_rollangles[i]);

						if (store->RMB_beam_data_available & 0x0100)
							fprintf(stderr, " %f", store->RMB_sounding_takeoffangles[i]);

						if (store->RMB_beam_data_available & 0x0200)
							fprintf(stderr, " %f", store->RMB_sounding_azimuthalangles[i]);

						if (store->RMB_beam_data_available & 0x0400)
							fprintf(stderr, " %d", store->RMB_sounding_timedelays[i]);

						if (store->RMB_beam_data_available & 0x0800)
							fprintf(stderr, " %d", store->RMB_sounding_intensities[i]);

						if (store->RMB_beam_data_available & 0x1000)
							fprintf(stderr, " %d", store->RMB_sounding_quality[i]);

						if (store->RMB_beam_data_available & 0x2000)
							fprintf(stderr, " %d", store->RMB_sounding_flags[i]);

						fprintf(stderr, "\n");
					}
				}

				/* check if this completes a survey ping */
				if (status == MB_SUCCESS) {
					if (store->SNR_ping_number > 0) {
						if (store->SNR_ping_number == store->RMB_ping_number)
							SNRok = MB_YES;
						else if (10 * store->SNR_ping_number == store->RMB_ping_number)
							SNRok = MB_YES;
						else
							SNRok = MB_NO;
					}
					else
						SNRok = MB_YES;
					if (store->RSS_ping_number > 0) {
						if (store->RSS_ping_number == store->RMB_ping_number)
							RSSok = MB_YES;
						else if (10 * store->RSS_ping_number == store->RMB_ping_number)
							RSSok = MB_YES;
						else
							RSSok = MB_NO;
					}
					else
						RSSok = MB_YES;
					if (SNRok == MB_YES && RSSok == MB_YES) {
						done = MB_YES;
						store->kind = MB_DATA_DATA;
						store->time_d = store->TND_survey_time_d + store->RMB_time;
						mb_get_date(verbose, store->time_d, store->time_i);
					}
					/* fprintf(stderr,"RMB SNRok:%d RSSok:%d done:%d store->kind:%d\n",SNRok,RSSok,done,store->kind); */
				}

				/* set *RMB_read flag */
				if (done == MB_NO && status == MB_SUCCESS)
					*RMB_read = MB_YES;
			}

			/* RSS data record */
			else if (strncmp(line, "RSS", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_RSS;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %x %d %d %lf %d %lf %lf %d %d %d %d", &(store->RSS_device_number),
				               &(store->RSS_time), &(store->RSS_sonar_flags), &(store->RSS_port_num_samples),
				               &(store->RSS_starboard_num_samples), &(store->RSS_sound_velocity), &(store->RSS_ping_number),
				               &(store->RSS_altitude), &(store->RSS_sample_rate), &(store->RSS_minimum_amplitude),
				               &(store->RSS_maximum_amplitude), &(store->RSS_bit_shift), &(store->RSS_frequency));
				if (nscan == 10) {
					store->RSS_minimum_amplitude = 0;
					store->RSS_maximum_amplitude = 0;
					store->RSS_frequency = 0;
				}
				else if (nscan == 12) {
					store->RSS_frequency = 0;
				}
				else if (nscan < 12) {
					store->RSS_port_num_samples = 0;
					store->RSS_starboard_num_samples = 0;
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* allocate space for sidescan data if required */
				if (status == MB_SUCCESS && store->RSS_port_num_samples > store->RSS_port_num_samples_alloc) {
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RSS_port_num_samples * sizeof(int),
					                     (void **)&(store->RSS_port), error);
					if (status == MB_SUCCESS)
						store->RSS_port_num_samples_alloc = store->RSS_port_num_samples;
					else
						store->RSS_port_num_samples_alloc = 0;
					;
				}
				if (status == MB_SUCCESS && store->RSS_starboard_num_samples > store->RSS_starboard_num_samples_alloc) {
					status = mb_reallocd(verbose, __FILE__, __LINE__, store->RSS_starboard_num_samples * sizeof(int),
					                     (void **)&(store->RSS_starboard), error);
					if (status == MB_SUCCESS)
						store->RSS_starboard_num_samples_alloc = store->RSS_starboard_num_samples;
					else
						store->RSS_starboard_num_samples_alloc = 0;
					;
				}

				/* read and parse RSS_port */
				if (status == MB_SUCCESS) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RSS_port_num_samples && token != NULL; i++) {
							nscan = sscanf(token, "%d", &(store->RSS_port[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RSS_port_num_samples) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse RSS_starboard */
				if (status == MB_SUCCESS) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->RSS_starboard_num_samples && token != NULL; i++) {
							nscan = sscanf(token, "%d", &(store->RSS_starboard[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->RSS_starboard_num_samples) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  RMB data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       RSS_device_number:                 %d\n", store->RSS_device_number);
					fprintf(stderr, "dbg4       RSS_time:                          %f\n", store->RSS_time);
					fprintf(stderr, "dbg4       RSS_sonar_flags:                   %x\n", store->RSS_sonar_flags);
					fprintf(stderr, "dbg4       RSS_port_num_samples:              %d\n", store->RSS_port_num_samples);
					fprintf(stderr, "dbg4       RSS_port_num_samples_alloc:        %d\n", store->RSS_port_num_samples_alloc);
					fprintf(stderr, "dbg4       RSS_starboard_num_samples:         %d\n", store->RSS_starboard_num_samples);
					fprintf(stderr, "dbg4       RSS_starboard_num_samples_alloc:   %d\n", store->RSS_starboard_num_samples_alloc);
					fprintf(stderr, "dbg4       RSS_sound_velocity:                %f\n", store->RSS_sound_velocity);
					fprintf(stderr, "dbg4       RSS_ping_number:                   %d\n", store->RSS_ping_number);
					fprintf(stderr, "dbg4       RSS_altitude:                      %f\n", store->RSS_altitude);
					fprintf(stderr, "dbg4       RSS_sample_rate:                   %f\n", store->RSS_sample_rate);
					fprintf(stderr, "dbg4       RSS_minimum_amplitude:             %d\n", store->RSS_minimum_amplitude);
					fprintf(stderr, "dbg4       RSS_maximum_amplitude:             %d\n", store->RSS_maximum_amplitude);
					fprintf(stderr, "dbg4       RSS_bit_shift:                     %d\n", store->RSS_bit_shift);
					fprintf(stderr, "dbg4       RSS_frequency:                     %d\n", store->RSS_frequency);
					for (i = 0; i < store->RSS_port_num_samples; i++) {
						fprintf(stderr, "dbg4       port pixel:%5d ss:%d\n", i, store->RSS_port[i]);
					}
					for (i = 0; i < store->RSS_port_num_samples; i++) {
						fprintf(stderr, "dbg4       starboard pixel:%5d ss:%d\n", i, store->RSS_starboard[i]);
					}
				}

				/* check if this completes a survey ping */
				if (status == MB_SUCCESS) {
					if (store->RSS_ping_number == store->RMB_ping_number)
						RSSok = MB_YES;
					else if (10 * store->RSS_ping_number == store->RMB_ping_number)
						RSSok = MB_YES;
					else
						RSSok = MB_NO;
					if (store->SNR_ping_number > 0) {
						if (store->SNR_ping_number == store->RMB_ping_number)
							SNRok = MB_YES;
						else if (10 * store->SNR_ping_number == store->RMB_ping_number)
							SNRok = MB_YES;
						else
							SNRok = MB_NO;
					}
					else
						SNRok = MB_YES;
					if (SNRok == MB_YES && RSSok == MB_YES) {
						done = MB_YES;
						store->kind = MB_DATA_DATA;
						store->time_d = store->TND_survey_time_d + store->RMB_time;
						mb_get_date(verbose, store->time_d, store->time_i);
					}
					/* fprintf(stderr,"RSS SNRok:%d RSSok:%d done:%d store->kind:%d\n",SNRok,RSSok,done,store->kind); */
				}

				/* set *RMB_read flag */
				if (done == MB_YES)
					*RMB_read = MB_NO;
			}

			/* MSS data record */
			else if (strncmp(line, "MSS", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_MSS;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %lf %d %lf %d", &(store->MSS_device_number), &(store->MSS_time),
				               &(store->MSS_sound_velocity), &(store->MSS_num_pixels), &(store->MSS_pixel_size),
				               &(store->MSS_ping_number));
				if (nscan != 6) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* read and parse MSS_ss */
				if (status == MB_SUCCESS) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->MSS_num_pixels && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->MSS_ss[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->MSS_num_pixels) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}
				}

				/* read and parse MSS_along */
				if (status == MB_SUCCESS) {
					/* read the next line */
					status = mbr_hysweep1_rd_line(verbose, mb_io_ptr->mbfp, line, error);

					/* parse the line */
					if (status == MB_SUCCESS && (token = strtok(line + 0, " ")) != NULL) {
						nread = 0;
						for (i = 0; i < store->MSS_num_pixels && token != NULL; i++) {
							nscan = sscanf(token, "%lf", &(store->MSS_ss_along[i]));
							if (nscan == 1)
								nread++;
							token = strtok(NULL, " ");
						}
						if (nread != store->MSS_num_pixels) {
							status = MB_FAILURE;
							*error = MB_ERROR_UNINTELLIGIBLE;
						}
					}

					/* calculate MSS_across */
					for (i = 0; i < store->MSS_num_pixels; i++) {
						store->MSS_ss_across[i] = store->MSS_pixel_size * (double)(i - (store->MSS_num_pixels / 2));
						;
					}
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  MSS data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       MSS_device_number:                 %d\n", store->MSS_device_number);
					fprintf(stderr, "dbg4       MSS_time:                          %f\n", store->MSS_time);
					fprintf(stderr, "dbg4       MSS_sound_velocity:                %f\n", store->MSS_sound_velocity);
					fprintf(stderr, "dbg4       MSS_num_pixels:                    %d\n", store->MSS_num_pixels);
					fprintf(stderr, "dbg4       MSS_pixel_size:                    %f\n", store->MSS_pixel_size);
					fprintf(stderr, "dbg4       MSS_ping_number:                   %d\n", store->MSS_ping_number);
					for (i = 0; i < store->MSS_num_pixels; i++) {
						fprintf(stderr, "dbg4       pixel:%5d ss:%f across:%f along:%f\n", i, store->MSS_ss[i],
						        store->MSS_ss_across[i], store->MSS_ss_along[i]);
					}
				}

				/* the MSS record is always written before RMB and so never completes a survey ping */
			}

			/* SNR data record */
			else if (strncmp(line, "SNR", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_SNR;

				/* parse the first line */
				nscan = sscanf(
				    line + 4, "%d %lf %d %d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", &(store->SNR_device_number),
				    &(store->SNR_time), &(store->SNR_ping_number), &(store->SNR_sonar_id), &(store->SNR_num_settings),
				    &(store->SNR_settings[0]), &(store->SNR_settings[1]), &(store->SNR_settings[2]), &(store->SNR_settings[3]),
				    &(store->SNR_settings[4]), &(store->SNR_settings[5]), &(store->SNR_settings[6]), &(store->SNR_settings[7]),
				    &(store->SNR_settings[8]), &(store->SNR_settings[9]), &(store->SNR_settings[10]), &(store->SNR_settings[11]));
				if (nscan != (5 + store->SNR_num_settings)) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  SNR data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       SNR_device_number:                 %d\n", store->SNR_device_number);
					fprintf(stderr, "dbg4       SNR_time:                          %f\n", store->SNR_time);
					fprintf(stderr, "dbg4       SNR_ping_number:                   %d\n", store->SNR_ping_number);
					fprintf(stderr, "dbg4       SNR_sonar_id:                      %d\n", store->SNR_sonar_id);
					fprintf(stderr, "dbg4       SNR_num_settings:                  %d\n", store->SNR_num_settings);
					fprintf(stderr, "dbg4       SNR_settings[0]:                   %f\n", store->SNR_settings[0]);
					fprintf(stderr, "dbg4       SNR_settings[1]:                   %f\n", store->SNR_settings[1]);
					fprintf(stderr, "dbg4       SNR_settings[2]:                   %f\n", store->SNR_settings[2]);
					fprintf(stderr, "dbg4       SNR_settings[3]:                   %f\n", store->SNR_settings[3]);
					fprintf(stderr, "dbg4       SNR_settings[4]:                   %f\n", store->SNR_settings[4]);
					fprintf(stderr, "dbg4       SNR_settings[5]:                   %f\n", store->SNR_settings[5]);
					fprintf(stderr, "dbg4       SNR_settings[6]:                   %f\n", store->SNR_settings[6]);
					fprintf(stderr, "dbg4       SNR_settings[7]:                   %f\n", store->SNR_settings[7]);
					fprintf(stderr, "dbg4       SNR_settings[8]:                   %f\n", store->SNR_settings[8]);
					fprintf(stderr, "dbg4       SNR_settings[9]:                   %f\n", store->SNR_settings[9]);
					fprintf(stderr, "dbg4       SNR_settings[10]:                  %f\n", store->SNR_settings[10]);
					fprintf(stderr, "dbg4       SNR_settings[11]:                  %f\n", store->SNR_settings[11]);
				}

				/* check if this completes a survey ping */
				if (status == MB_SUCCESS) {
					if (store->SNR_ping_number == store->RMB_ping_number)
						SNRok = MB_YES;
					else if (10 * store->SNR_ping_number == store->RMB_ping_number)
						SNRok = MB_YES;
					else
						SNRok = MB_NO;
					if (store->RSS_ping_number > 0) {
						if (store->RSS_ping_number == store->RMB_ping_number)
							RSSok = MB_YES;
						else if (10 * store->RSS_ping_number == store->RMB_ping_number)
							RSSok = MB_YES;
						else
							RSSok = MB_NO;
					}
					else
						RSSok = MB_YES;
					if (SNRok == MB_YES && RSSok == MB_YES) {
						done = MB_YES;
						store->kind = MB_DATA_DATA;
						store->time_d = store->TND_survey_time_d + store->RMB_time;
						mb_get_date(verbose, store->time_d, store->time_i);
					}
				}

				/* set *RMB_read flag */
				if (done == MB_YES)
					*RMB_read = MB_NO;
			}

			/* TID data record */
			else if (strncmp(line, "TID", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_TID;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %lf", &(store->TID_device_number), &(store->TID_time), &(store->TID_tide));
				if (nscan != 3) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  TID data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       TID_device_number:                 %d\n", store->TID_device_number);
					fprintf(stderr, "dbg4       TID_time:                          %f\n", store->TID_time);
					fprintf(stderr, "dbg4       TID_tide:                          %f\n", store->TID_tide);
				}

				/* if successful this completes a tide record */
				if (status == MB_SUCCESS) {
					done = MB_YES;
					store->kind = MB_DATA_TIDE;
					store->time_d = store->TND_survey_time_d + store->TID_time;
					mb_get_date(verbose, store->time_d, store->time_i);
				}
			}

			/* HCP data record */
			else if (strncmp(line, "HCP", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_HCP;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %lf %lf %lf", &(store->HCP_device_number), &(store->HCP_time),
				               &(store->HCP_heave), &(store->HCP_roll), &(store->HCP_pitch));
				if (nscan != 5) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  HCP data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       HCP_device_number:                 %d\n", store->HCP_device_number);
					fprintf(stderr, "dbg4       HCP_time:                          %f\n", store->HCP_time);
					fprintf(stderr, "dbg4       HCP_heave:                         %f\n", store->HCP_heave);
					fprintf(stderr, "dbg4       HCP_roll:                          %f\n", store->HCP_roll);
					fprintf(stderr, "dbg4       HCP_pitch:                         %f\n", store->HCP_pitch);
				}

				/* if successful this completes an attitude record */
				if (status == MB_SUCCESS) {
					done = MB_YES;
					store->kind = MB_DATA_ATTITUDE;
					store->time_d = store->TND_survey_time_d + store->HCP_time;
					mb_get_date(verbose, store->time_d, store->time_i);
				}
			}

			/* EC1 data record */
			else if (strncmp(line, "EC1", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_EC1;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %lf", &(store->EC1_device_number), &(store->EC1_time), &(store->EC1_rawdepth));
				if (nscan != 3) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  EC1 data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       EC1_device_number:                 %d\n", store->EC1_device_number);
					fprintf(stderr, "dbg4       EC1_time:                          %f\n", store->EC1_time);
					fprintf(stderr, "dbg4       EC1_rawdepth:                      %f\n", store->EC1_rawdepth);
				}

				/* if successful this completes an altitude record */
				if (status == MB_SUCCESS) {
					done = MB_YES;
					store->kind = MB_DATA_ALTITUDE;
					store->time_d = store->TND_survey_time_d + store->EC1_time;
					mb_get_date(verbose, store->time_d, store->time_i);
				}
			}

			/* GPS data record */
			else if (strncmp(line, "GPS", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_GPS;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %lf %lf %lf %d %d", &(store->GPS_device_number), &(store->GPS_time),
				               &(store->GPS_cog), &(store->GPS_sog), &(store->GPS_hdop), &(store->GPS_mode), &(store->GPS_nsats));
				if (nscan != 7) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  GPS data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       GPS_device_number:                 %d\n", store->GPS_device_number);
					fprintf(stderr, "dbg4       GPS_time:                          %f\n", store->GPS_time);
					fprintf(stderr, "dbg4       GPS_cog:                           %f\n", store->GPS_cog);
					fprintf(stderr, "dbg4       GPS_sog:                           %f\n", store->GPS_sog);
					fprintf(stderr, "dbg4       GPS_hdop:                          %f\n", store->GPS_hdop);
					fprintf(stderr, "dbg4       GPS_mode:                          %d\n", store->GPS_mode);
					fprintf(stderr, "dbg4       GPS_nsats:                         %d\n", store->GPS_nsats);
				}
			}

			/* GYR data record */
			else if (strncmp(line, "GYR", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_GYR;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %lf", &(store->GYR_device_number), &(store->GYR_time), &(store->GYR_heading));
				if (nscan != 3) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  GYR data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       GYR_device_number:                 %d\n", store->GYR_device_number);
					fprintf(stderr, "dbg4       GYR_time:                          %f\n", store->GYR_time);
					fprintf(stderr, "dbg4       GYR_heading:                       %f\n", store->GYR_heading);
				}

				/* if successful this completes a heading record */
				if (status == MB_SUCCESS) {
					done = MB_YES;
					store->kind = MB_DATA_HEADING;
					store->time_d = store->TND_survey_time_d + store->GYR_time;
					mb_get_date(verbose, store->time_d, store->time_i);
				}
			}

			/* POS data record */
			else if (strncmp(line, "POS", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_POS;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %lf %lf", &(store->POS_device_number), &(store->POS_time), &(store->POS_x),
				               &(store->POS_y));
				if (nscan != 4) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  POS data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       POS_device_number:                 %d\n", store->POS_device_number);
					fprintf(stderr, "dbg4       POS_time:                          %f\n", store->POS_time);
					fprintf(stderr, "dbg4       POS_x:                             %f\n", store->POS_x);
					fprintf(stderr, "dbg4       POS_y:                             %f\n", store->POS_y);
				}

				/* if successful this completes a navigation record */
				if (status == MB_SUCCESS) {
					done = MB_YES;
					device = (struct mbsys_hysweep_device_struct *)&(store->devices[store->POS_device_number]);
					if (device->DV2_enabled == MB_YES)
						store->kind = MB_DATA_NAV;
					else
						store->kind = MB_DATA_NAV1;
					store->time_d = store->TND_survey_time_d + store->POS_time;
					mb_get_date(verbose, store->time_d, store->time_i);
				}
			}

			/* DEV device data record */
			else if (strncmp(line, "DEV", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_DEV;

				/* parse the first line */
				token = strtok(line + 4, " ");
				nscan = sscanf(token, "%d", &(DEV_device_number));
				if (nscan == 1) {
					token = strtok(NULL, " ");
					nscan = sscanf(token, "%d", &(DEV_device_capability));
				}
				if (nscan == 1) {
					token = strtok(NULL, "\"");
					strcpy(DEV_device_name, token);
					nscan = 3;
				}
				if (nscan == 3) {
					device = (struct mbsys_hysweep_device_struct *)&(store->devices[DEV_device_number]);
					device->DEV_device_number = DEV_device_number;
					device->DEV_device_capability = DEV_device_capability;
					strcpy(device->DEV_device_name, DEV_device_name);
					store->num_devices++;
				}
				else {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  DEV data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       DEV_device_number:                 %d\n", device->DEV_device_number);
					fprintf(stderr, "dbg4       DEV_device_capability:             %d\n", device->DEV_device_capability);
					fprintf(stderr, "dbg4       DEV_device_name:                   %s\n", device->DEV_device_name);
				}
			}

			/* DV2 device data record */
			else if (strncmp(line, "DV2", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_DV2;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %x %d %d", &(DV2_device_number), &(DV2_device_capability), &(DV2_towfish),
				               &(DV2_enabled));
				if (nscan == 4) {
					device = (struct mbsys_hysweep_device_struct *)&(store->devices[DEV_device_number]);
					device->DV2_device_capability = DV2_device_capability;
					device->DV2_towfish = DV2_towfish;
					device->DV2_enabled = DV2_enabled;
				}
				else {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  DV2 data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       DV2_device_number:                 %d\n", DV2_device_number);
					fprintf(stderr, "dbg4       DV2_device_capability:             %d\n", device->DV2_device_capability);
					fprintf(stderr, "dbg4       DV2_towfish:                       %d\n", device->DV2_towfish);
					fprintf(stderr, "dbg4       DV2_enabled:                       %d\n", device->DV2_enabled);
				}
			}

			/* EOH end of header data record */
			else if (strncmp(line, "EOH", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_EOH;

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  EOH data record read by MBIO function <%s>\n", function_name);
				}

				/* handle some bookkeeping since the header has all been read */
				*file_header_read = MB_YES;

				/* handle projection - if one is already initialized from
				   a *.prj file leave it in place, if not use the first
				   PRJ record in the file, if no projection set just use
				   UTM01N */
				if (mb_io_ptr->projection_initialized == MB_YES) {
					strcpy(store->PRJ_proj4_command, mb_io_ptr->projection_id);
				}
				else {
					/* if no projection set just use UTM01N */
					if (strlen(store->PRJ_proj4_command) == 0) {
						strcpy(store->PRJ_proj4_command, "UTM01N");
					}

					/* initialize the projection */
					mb_proj_init(verbose, store->PRJ_proj4_command, &(mb_io_ptr->pjptr), error);
					strcpy(mb_io_ptr->projection_id, store->PRJ_proj4_command);
					mb_io_ptr->projection_initialized = MB_YES;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  Hysweep file header read by MBIO function <%s>\n", function_name);
				}

				/* if successful this completes a file header record */
				if (status == MB_SUCCESS) {
					done = MB_YES;
					store->kind = MB_DATA_HEADER;
				}
			}

			/* EOL end of line data record */
			else if (strncmp(line, "EOL", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_EOL;

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  EOL data record read by MBIO function <%s>\n", function_name);
				}
			}

			/* FTP file type data record */
			else if (strncmp(line, "FTP", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_FTP;

				/* parse the first line */
				strcpy(store->FTP_record, line + 4);
				len = strlen(store->FTP_record);
				if (len <= 0) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}
				else {
					for (i = 0; i < len; i++) {
						if (store->FTP_record[i] == '\r' || store->FTP_record[i] == '\n')
							store->FTP_record[i] = '\0';
					}
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  FTP data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       FTP_record:                        %s\n", store->FTP_record);
				}
			}

			/* VER file type data record */
			else if (strncmp(line, "VER", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_VER;

				/* parse the first line */
				nscan = sscanf(line + 4, "%s", store->VER_version);
				if (nscan != 1) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  VER data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       VER_version:                       %s\n", store->VER_version);
				}
			}

			/* HSP data record */
			else if (strncmp(line, "HSP", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_HSP;

				/* parse the first line */
				nscan = sscanf(line + 4, "%lf %lf %lf %lf %lf %lf %d %d %lf %lf %d %d", &(store->HSP_minimum_depth),
				               &(store->HSP_maximum_depth), &(store->HSP_port_offset_limit), &(store->HSP_stbd_offset_limit),
				               &(store->HSP_port_angle_limit), &(store->HSP_stbd_angle_limit), &(store->HSP_high_beam_quality),
				               &(store->HSP_low_beam_quality), &(store->HSP_sonar_range), &(store->HSP_towfish_layback),
				               &(store->HSP_units), &(store->HSP_sonar_id));
				if (nscan != 12) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  HSP data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       HSP_minimum_depth:                 %f\n", store->HSP_minimum_depth);
					fprintf(stderr, "dbg4       HSP_maximum_depth:                 %f\n", store->HSP_maximum_depth);
					fprintf(stderr, "dbg4       HSP_port_offset_limit:             %f\n", store->HSP_port_offset_limit);
					fprintf(stderr, "dbg4       HSP_stbd_offset_limit:             %f\n", store->HSP_stbd_offset_limit);
					fprintf(stderr, "dbg4       HSP_port_angle_limit:              %f\n", store->HSP_port_angle_limit);
					fprintf(stderr, "dbg4       HSP_stbd_angle_limit:              %f\n", store->HSP_stbd_angle_limit);
					fprintf(stderr, "dbg4       HSP_high_beam_quality:             %d\n", store->HSP_high_beam_quality);
					fprintf(stderr, "dbg4       HSP_low_beam_quality:              %d\n", store->HSP_low_beam_quality);
					fprintf(stderr, "dbg4       HSP_sonar_range:                   %f\n", store->HSP_sonar_range);
					fprintf(stderr, "dbg4       HSP_towfish_layback:               %f\n", store->HSP_towfish_layback);
					fprintf(stderr, "dbg4       HSP_units:                         %d\n", store->HSP_units);
					fprintf(stderr, "dbg4       HSP_sonar_id:                      %d\n", store->HSP_sonar_id);
				}
			}

			/* HSX data record */
			else if (strncmp(line, "HSX", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_HSX;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d", &(store->HSX_record));
				if (nscan != 1) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  HSX data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       HSX_record:                        %d\n", store->HSX_record);
				}
			}

			/* HVF data record */
			else if (strncmp(line, "HVF", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_HVF;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %lf %lf %lf %lf %lf %lf", &(HVF_device_number),
				               &(store->HVF_time_after_midnight), &(store->HVF_minimum_depth), &(store->HVF_maximum_depth),
				               &(store->HVF_port_offset_limit), &(store->HVF_starboard_offset_limit),
				               &(store->HVF_minimum_angle_limit), &(store->HVF_maximum_angle_limit));
				if (nscan != 8) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  HVF data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       HVF_device_number:                 %d\n", HVF_device_number);
					fprintf(stderr, "dbg4       HVF_time_after_midnight:           %f\n", store->HVF_time_after_midnight);
					fprintf(stderr, "dbg4       HVF_minimum_depth:                 %f\n", store->HVF_minimum_depth);
					fprintf(stderr, "dbg4       HVF_maximum_depth:                 %f\n", store->HVF_maximum_depth);
					fprintf(stderr, "dbg4       HVF_port_offset_limit:             %f\n", store->HVF_port_offset_limit);
					fprintf(stderr, "dbg4       HVF_starboard_offset_limit:        %f\n", store->HVF_starboard_offset_limit);
					fprintf(stderr, "dbg4       HVF_minimum_angle_limit:           %f\n", store->HVF_minimum_angle_limit);
					fprintf(stderr, "dbg4       HVF_maximum_angle_limit:           %f\n", store->HVF_maximum_angle_limit);
				}
			}

			/* INF data record */
			else if (strncmp(line, "INF", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_INF;

				/* parse the first line */
				if (strncmp(line, "INF \"\" \"\" \"\" \"\" ", 16) == 0) {
					store->INF_surveyor[0] = '\0';
					store->INF_boat[0] = '\0';
					store->INF_project[0] = '\0';
					store->INF_area[0] = '\0';
					nscan = 0;
					if (strlen(line) > 16)
						nscan = sscanf(&(line[16]), "%lf %lf %lf", &(store->INF_tide_correction), &(store->INF_draft_correction),
						               &(store->INF_sound_velocity));
					if (nscan < 3)
						store->INF_sound_velocity = 1500.0;
					if (nscan < 2)
						store->INF_draft_correction = 0.0;
					if (nscan < 1)
						store->INF_tide_correction = 0.0;
				}
				else {
					if ((token = strtok(line + 3, "\"")) != NULL) {
						token = strtok(NULL, "\"");
						if (token != NULL && token[0] != ' ') {
							strcpy(store->INF_surveyor, token);
							token = strtok(NULL, "\"");
						}
						else
							store->INF_surveyor[0] = '\0';

						token = strtok(NULL, "\"");
						if (token != NULL && token[0] != ' ') {
							strcpy(store->INF_boat, token);
							token = strtok(NULL, "\"");
						}
						else
							store->INF_boat[0] = '\0';

						token = strtok(NULL, "\"");
						if (token != NULL && token[0] != ' ') {
							strcpy(store->INF_project, token);
							token = strtok(NULL, "\"");
						}
						else
							store->INF_project[0] = '\0';

						token = strtok(NULL, "\"");
						if (token != NULL && token[0] != ' ') {
							strcpy(store->INF_area, token);
							token = strtok(NULL, "\"");
						}
						else
							store->INF_area[0] = '\0';
					}
					nscan = 0;
					if (token != NULL)
						nscan = sscanf(token, "%lf %lf %lf", &(store->INF_tide_correction), &(store->INF_draft_correction),
						               &(store->INF_sound_velocity));
					if (nscan < 3)
						store->INF_sound_velocity = 1500.0;
					if (nscan < 2)
						store->INF_draft_correction = 0.0;
					if (nscan < 1)
						store->INF_tide_correction = 0.0;
				}
				if (nscan != 3) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  INF data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       INF_surveyor:                      %s\n", store->INF_surveyor);
					fprintf(stderr, "dbg4       INF_boat:                          %s\n", store->INF_boat);
					fprintf(stderr, "dbg4       INF_project:                       %s\n", store->INF_project);
					fprintf(stderr, "dbg4       INF_area:                          %s\n", store->INF_area);
					fprintf(stderr, "dbg4       INF_tide_correction:               %f\n", store->INF_tide_correction);
					fprintf(stderr, "dbg4       INF_draft_correction:              %f\n", store->INF_draft_correction);
					fprintf(stderr, "dbg4       INF_sound_velocity:                %f\n", store->INF_sound_velocity);
				}
			}

			/* LBP data record */
			else if (strncmp(line, "LBP", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_LBP;

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  LBP data record read by MBIO function <%s>\n", function_name);
				}
			}

			/* LIN data record */
			else if (strncmp(line, "LIN", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_LIN;

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  LIN data record read by MBIO function <%s>\n", function_name);
				}
			}

			/* LNN data record */
			else if (strncmp(line, "LNN", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_LNN;

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  LNN data record read by MBIO function <%s>\n", function_name);
				}
			}

			/* MBI data record */
			else if (strncmp(line, "MBI", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_MBI;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %x %x %x %d %d %lf %lf", &(MBI_device_number), &(MBI_sonar_type), &(MBI_sonar_flags),
				               &(MBI_beam_data_available), &(MBI_num_beams_1), &(MBI_num_beams_2), &(MBI_first_beam_angle),
				               &(MBI_angle_increment));
				if (nscan == 8) {
					device = (struct mbsys_hysweep_device_struct *)&(store->devices[MBI_device_number]);
					device->MBI_sonar_type = MBI_sonar_type;
					device->MBI_sonar_flags = MBI_sonar_flags;
					device->MBI_beam_data_available = MBI_beam_data_available;
					device->MBI_num_beams_1 = MBI_num_beams_1;
					device->MBI_num_beams_2 = MBI_num_beams_2;
					device->MBI_first_beam_angle = MBI_first_beam_angle;
					device->MBI_angle_increment = MBI_angle_increment;
				}
				else {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  MBI data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       MBI_device_number:                 %d\n", MBI_device_number);
					fprintf(stderr, "dbg4       MBI_sonar_id:                      %d\n", device->MBI_sonar_id);
					fprintf(stderr, "dbg4       MBI_sonar_receive_shape:           %d\n", device->MBI_sonar_receive_shape);
					fprintf(stderr, "dbg4       MBI_sonar_type:                    %x\n", device->MBI_sonar_type);
					fprintf(stderr, "dbg4       MBI_sonar_flags:                   %x\n", device->MBI_sonar_flags);
					fprintf(stderr, "dbg4       MBI_beam_data_available:           %x\n", device->MBI_beam_data_available);
					fprintf(stderr, "dbg4       MBI_num_beams_1:                   %d\n", device->MBI_num_beams_1);
					fprintf(stderr, "dbg4       MBI_num_beams_2:                   %d\n", device->MBI_num_beams_2);
					fprintf(stderr, "dbg4       MBI_first_beam_angle:              %f\n", device->MBI_first_beam_angle);
					fprintf(stderr, "dbg4       MBI_angle_increment:               %f\n", device->MBI_angle_increment);
				}
			}

			/* OF2 data record */
			else if (strncmp(line, "OF2", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_OF2;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %d %lf %lf %lf %lf %lf %lf %lf", &(OF2_device_number), &(OF2_offset_type),
				               &(OF2_offset_starboard), &(OF2_offset_forward), &(OF2_offset_vertical), &(OF2_offset_yaw),
				               &(OF2_offset_roll), &(OF2_offset_pitch), &(OF2_offset_time));
				if (nscan == 9) {
					device = (struct mbsys_hysweep_device_struct *)&(store->devices[OF2_device_number]);
					offset = (struct mbsys_hysweep_device_offset_struct *)&(device->offsets[device->num_offsets]);
					offset->OF2_device_number = OF2_device_number;
					offset->OF2_offset_type = OF2_offset_type;
					offset->OF2_offset_starboard = OF2_offset_starboard;
					offset->OF2_offset_forward = OF2_offset_forward;
					offset->OF2_offset_vertical = OF2_offset_vertical;
					offset->OF2_offset_yaw = OF2_offset_yaw;
					offset->OF2_offset_roll = OF2_offset_roll;
					offset->OF2_offset_pitch = OF2_offset_pitch;
					offset->OF2_offset_time = OF2_offset_time;
					device->num_offsets++;
				}
				else {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  OF2 data record read by OF2O function <%s>\n", function_name);
					fprintf(stderr, "dbg4       OF2_device_number:                 %d\n", OF2_device_number);
					fprintf(stderr, "dbg4       num_offsets:                       %d\n", device->num_offsets);
					fprintf(stderr, "dbg4       OF2_offset_type:                   %d\n", offset->OF2_offset_type);
					fprintf(stderr, "dbg4       OF2_offset_starboard:              %f\n", offset->OF2_offset_starboard);
					fprintf(stderr, "dbg4       OF2_offset_forward:                %f\n", offset->OF2_offset_forward);
					fprintf(stderr, "dbg4       OF2_offset_vertical:               %f\n", offset->OF2_offset_vertical);
					fprintf(stderr, "dbg4       OF2_offset_yaw:                    %f\n", offset->OF2_offset_yaw);
					fprintf(stderr, "dbg4       OF2_offset_roll:                   %f\n", offset->OF2_offset_roll);
					fprintf(stderr, "dbg4       OF2_offset_pitch:                  %f\n", offset->OF2_offset_pitch);
					fprintf(stderr, "dbg4       OF2_offset_time:                   %f\n", offset->OF2_offset_time);
				}
			}

			/* PRI data record */
			else if (strncmp(line, "PRI", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_PRI;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d", &(store->primary_nav_device));
				if (nscan == 1) {
					device = (struct mbsys_hysweep_device_struct *)&(store->devices[store->primary_nav_device]);
					device->PRI_primary_nav_device = 1;
				}
				else {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  PRI data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       primary_nav_device:                %d\n", store->primary_nav_device);
				}
			}

			/* PTS data record */
			else if (strncmp(line, "PTS", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_PTS;

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  PTS data record read by MBIO function <%s>\n", function_name);
				}
			}

			/* SSI data record */
			else if (strncmp(line, "SSI", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_SSI;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %x %d %d", &(SSI_device_number), &(SSI_sonar_flags), &(SSI_port_num_samples),
				               &(SSI_starboard_num_samples));
				if (nscan == 4) {
					device = (struct mbsys_hysweep_device_struct *)&(store->devices[SSI_device_number]);
					device->SSI_sonar_flags = SSI_sonar_flags;
					device->SSI_port_num_samples = SSI_port_num_samples;
					device->SSI_starboard_num_samples = SSI_starboard_num_samples;
				}
				else {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  SSI data record read by SSIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       SSI_device_number:                 %d\n", SSI_device_number);
					fprintf(stderr, "dbg4       SSI_sonar_flags:                   %x\n", device->SSI_sonar_flags);
					fprintf(stderr, "dbg4       SSI_port_num_samples:              %d\n", device->SSI_port_num_samples);
					fprintf(stderr, "dbg4       SSI_starboard_num_samples:         %d\n", device->SSI_starboard_num_samples);
				}
			}

			/* SVC data record */
			else if (strncmp(line, "SVC", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_SVC;

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  SVC data record read by MBIO function <%s>\n", function_name);
				}
			}

			/* TND data record */
			else if (strncmp(line, "TND", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_TND;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d:%d:%d %d/%d/%d", &(store->TND_survey_time_i[3]), &(store->TND_survey_time_i[4]),
				               &(store->TND_survey_time_i[5]), &(store->TND_survey_time_i[1]), &(store->TND_survey_time_i[2]),
				               &(store->TND_survey_time_i[0]));
				if (nscan == 6) {
					store->TND_survey_time_i[6] = 0;
					store->time_i[0] = store->TND_survey_time_i[0];
					store->time_i[1] = store->TND_survey_time_i[1];
					store->time_i[2] = store->TND_survey_time_i[2];
					store->time_i[3] = 0;
					store->time_i[4] = 0;
					store->time_i[5] = 0;
					store->time_i[6] = 0;
					mb_get_time(verbose, store->time_i, &(store->TND_survey_time_d));
				}
				else {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  TND data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       TND_survey_time_i[0]:              %d\n", store->TND_survey_time_i[0]);
					fprintf(stderr, "dbg4       TND_survey_time_i[1]:              %d\n", store->TND_survey_time_i[1]);
					fprintf(stderr, "dbg4       TND_survey_time_i[2]:              %d\n", store->TND_survey_time_i[2]);
					fprintf(stderr, "dbg4       TND_survey_time_i[3]:              %d\n", store->TND_survey_time_i[3]);
					fprintf(stderr, "dbg4       TND_survey_time_i[4]:              %d\n", store->TND_survey_time_i[4]);
					fprintf(stderr, "dbg4       TND_survey_time_i[5]:              %d\n", store->TND_survey_time_i[5]);
					fprintf(stderr, "dbg4       TND_survey_time_i[6]:              %d\n", store->TND_survey_time_i[6]);
					fprintf(stderr, "dbg4       TND_survey_time_d:                 %f\n", store->TND_survey_time_d);
				}
			}

			/* DFT data record */
			else if (strncmp(line, "DFT", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_DFT;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %lf", &(store->DFT_device_number), &(store->DFT_time), &(store->DFT_draft));
				if (nscan != 3) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  DFT data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       DFT_device_number:                 %d\n", store->DFT_device_number);
					fprintf(stderr, "dbg4       DFT_time:                          %f\n", store->DFT_time);
					fprintf(stderr, "dbg4       DFT_draft:                         %f\n", store->DFT_draft);
				}

				/* if successful this completes a sonar depth record */
				if (status == MB_SUCCESS) {
					done = MB_YES;
					store->kind = MB_DATA_SONARDEPTH;
					store->time_d = store->TND_survey_time_d + store->DFT_time;
					mb_get_date(verbose, store->time_d, store->time_i);
				}
			}

			/* FIX data record */
			else if (strncmp(line, "FIX", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_FIX;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %d", &(store->FIX_device_number), &(store->FIX_time_after_midnight),
				               &(store->FIX_event_number));
				if (nscan != 3) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  FIX data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       FIX_device_number:                 %d\n", store->FIX_device_number);
					fprintf(stderr, "dbg4       FIX_time_after_midnight:           %f\n", store->FIX_time_after_midnight);
					fprintf(stderr, "dbg4       FIX_event_number:                  %d\n", store->FIX_event_number);
				}
			}

			/* PSA data record */
			else if (strncmp(line, "PSA", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_PSA;

				/* parse the first line */
				nscan = sscanf(line + 4, "%d %lf %d %lf %lf", &(store->PSA_device_number), &(store->PSA_time),
				               &(store->PSA_ping_number), &(store->PSA_a0), &(store->PSA_a1));
				if (nscan != 5) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  PSA data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       PSA_device_number:                 %d\n", store->PSA_device_number);
					fprintf(stderr, "dbg4       PSA_time:                          %f\n", store->PSA_time);
					fprintf(stderr, "dbg4       PSA_ping_number:                   %d\n", store->PSA_ping_number);
					fprintf(stderr, "dbg4       PSA_a0:                            %f\n", store->PSA_a0);
					fprintf(stderr, "dbg4       PSA_a1:                            %f\n", store->PSA_a1);
				}
			}

			/* COM data record */
			else if (strncmp(line, "COM", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_COM;

				/* parse the first line */
				nscan = sscanf(line + 4, "%s", store->COM_comment);
				if (nscan != 1) {
					status = MB_FAILURE;
					*error = MB_ERROR_UNINTELLIGIBLE;
				}

				/* print debug statements */
				if (verbose >= 4) {
					fprintf(stderr, "\ndbg4  COM data record read by MBIO function <%s>\n", function_name);
					fprintf(stderr, "dbg4       COM_comment:                       %s\n", store->COM_comment);
				}
			}

			/* PRJ data record */
			else if (strncmp(line, "PRJ", 3) == 0) {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_PRJ;

				/* if no previously set projection use this one */
				if (strlen(store->PRJ_proj4_command) == 0) {
					/* parse the first line */
					nscan = sscanf(line + 4, "%s", store->PRJ_proj4_command);
					if (nscan != 1) {
						status = MB_FAILURE;
						*error = MB_ERROR_UNINTELLIGIBLE;
					}

					/* print debug statements */
					if (verbose >= 4) {
						fprintf(stderr, "\ndbg4  PRJ data record read by MBIO function <%s>\n", function_name);
						fprintf(stderr, "dbg4       PRJ_proj4_command:                 %s\n", store->PRJ_proj4_command);
					}
				}

				/* do not overwrite previously set projection */
				else {
					/* print debug statements */
					if (verbose >= 4) {
						fprintf(stderr, "\ndbg4  PRJ data record ignored by MBIO function <%s>\n", function_name);
						fprintf(stderr, "dbg4       Previously set PRJ_proj4_command:  %s\n", store->PRJ_proj4_command);
						fprintf(stderr, "dbg4       Ignored PRJ_proj4_command:         %s\n", line + 4);
					}
				}
			}

			/* unknown data record */
			else {
				store->type = MBSYS_HYSWEEP_RECORDTYPE_NONE;
			}

			/* treat unintelligible records as unknown */
			if (status == MB_FAILURE && *error == MB_ERROR_UNINTELLIGIBLE) {
				status = MB_SUCCESS;
				*error = MB_ERROR_NO_ERROR;
				store->type = MBSYS_HYSWEEP_RECORDTYPE_NONE;
			}
		}
	}
#ifdef MBR_HYSWEEP1_DEBUG
	if (status == MB_SUCCESS)
		fprintf(stderr, "HYSWEEP1 DATA READ: type:%d status:%d error:%d\n\n", store->kind, status, *error);
#endif

	/* get file position */
	mb_io_ptr->file_bytes = ftell(mb_io_ptr->mbfp);

	/* print output debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", function_name);
		fprintf(stderr, "dbg2  Return values:\n");
		fprintf(stderr, "dbg2       error:      %d\n", *error);
		fprintf(stderr, "dbg2  Return status:\n");
		fprintf(stderr, "dbg2       status:  %d\n", status);
	}

	/* return status */
	return (status);
}
/*--------------------------------------------------------------------*/
int mbr_hysweep1_rd_line(int verbose, FILE *mbfp, char *line, int *error) {
	char *function_name = "mbr_hysweep1_rd_line";
	int status = MB_SUCCESS;
	char *result;

	/* print input debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", function_name);
		fprintf(stderr, "dbg2  Revision id: %s\n", rcs_id);
		fprintf(stderr, "dbg2  Input arguments:\n");
		fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
		fprintf(stderr, "dbg2       mbfp:       %p\n", (void *)mbfp);
	}

	/* read the next line */
	result = fgets(line, MBSYS_HYSWEEP_MAXLINE, mbfp);
	if (result == line && strlen(line) < MBSYS_HYSWEEP_MAXLINE) {
		status = MB_SUCCESS;
		*error = MB_ERROR_NO_ERROR;

		if (verbose >= 4) {
			fprintf(stderr, "\ndbg4  Raw line read by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       line: %s\n", line);
		}
	}
	else {
		status = MB_FAILURE;
		*error = MB_ERROR_EOF;
	}

	/* print output debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", function_name);
		fprintf(stderr, "dbg2  Return values:\n");
		fprintf(stderr, "dbg2       line:       %s\n", line);
		fprintf(stderr, "dbg2       error:      %d\n", *error);
		fprintf(stderr, "dbg2  Return status:\n");
		fprintf(stderr, "dbg2       status:  %d\n", status);
	}

	/* return status */
	return (status);
}
/*--------------------------------------------------------------------*/
int mbr_hysweep1_wr_data(int verbose, void *mbio_ptr, void *store_ptr, int *error) {
	char *function_name = "mbr_hysweep1_wr_data";
	int status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;
	struct mbsys_hysweep_struct *store;
	struct mbsys_hysweep_device_struct *device;
	struct mbsys_hysweep_device_offset_struct *offset;
	struct mbsys_hysweep_struct hysweeptmp;
	FILE *mbfp;
	char *line;
	int *file_header_read;
	int *file_header_written;
	int kindex;
	int *add_MB_POS;
	int *add_MB_HCP;
	int *add_MB_GYR;
	int *add_MB_DFT;
	int *device_number_MB_POS;
	int *device_number_MB_HCP;
	int *device_number_MB_GYR;
	int *device_number_MB_DFT;
	int i, j;

	/* print input debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> called\n", function_name);
		fprintf(stderr, "dbg2  Revision id: %s\n", rcs_id);
		fprintf(stderr, "dbg2  Input arguments:\n");
		fprintf(stderr, "dbg2       verbose:    %d\n", verbose);
		fprintf(stderr, "dbg2       mbio_ptr:   %p\n", (void *)mbio_ptr);
		fprintf(stderr, "dbg2       store_ptr:  %p\n", (void *)store_ptr);
	}

	/* get pointer to mbio descriptor */
	mb_io_ptr = (struct mb_io_struct *)mbio_ptr;
	mbfp = mb_io_ptr->mbfp;

	/* get pointer to raw data structure */
	store = (struct mbsys_hysweep_struct *)store_ptr;
	line = store->writeline;

	/* get saved values */
	file_header_read = (int *)&mb_io_ptr->save1;
	file_header_written = (int *)&mb_io_ptr->save2;
	add_MB_POS = (int *)&mb_io_ptr->save3;
	add_MB_HCP = (int *)&mb_io_ptr->save4;
	add_MB_GYR = (int *)&mb_io_ptr->save5;
	add_MB_DFT = (int *)&mb_io_ptr->save6;
	device_number_MB_POS = (int *)&mb_io_ptr->save7;
	device_number_MB_HCP = (int *)&mb_io_ptr->save8;
	device_number_MB_GYR = (int *)&mb_io_ptr->save9;
	device_number_MB_DFT = (int *)&mb_io_ptr->save10;

	/* write fileheader if needed */
	if (*file_header_written == MB_NO && store->kind != MB_DATA_COMMENT) {
		/* copy the mbsys_hysweep structure so that we can mess with the device list
		    if needed before writing it out */
		hysweeptmp = *store;

		/* check for existence of MB-System interpolated nav, attitude, heading, and sonar depth devices
		    - create if they don't exist */
		*add_MB_POS = MB_YES;
		*add_MB_HCP = MB_YES;
		*add_MB_GYR = MB_YES;
		*add_MB_DFT = MB_YES;
		for (i = 0; i < hysweeptmp.num_devices; i++) {
			device = (struct mbsys_hysweep_device_struct *)&(hysweeptmp.devices[i]);
			if (device->DV2_enabled == MB_YES) {
				if ((strncmp(device->DEV_device_name, "MB-System", 9) == 0)) {
					if (device->DV2_device_capability & 0x0004)
						*add_MB_POS = MB_NO;
					if (device->DV2_device_capability & 0x0020)
						*add_MB_GYR = MB_NO;
					if (device->DV2_device_capability & 0x0200)
						*add_MB_HCP = MB_NO;
					if (device->DV2_device_capability == 0x8000)
						*add_MB_DFT = MB_NO;
				}
				else {
					if (device->DV2_device_capability & 0x0004) {
						device->DV2_enabled = MB_NO;
					}
					if (device->DV2_device_capability & 0x0020) {
						device->DV2_enabled = MB_NO;
					}
					if (device->DV2_device_capability & 0x0200) {
						device->DV2_enabled = MB_NO;
					}
					if (device->DV2_device_capability == 0x8000) {
						device->DV2_enabled = MB_NO;
					}
				}
			}
		}
		if (*add_MB_POS == MB_YES) {
			*device_number_MB_POS = hysweeptmp.num_devices;
			device = (struct mbsys_hysweep_device_struct *)&(hysweeptmp.devices[hysweeptmp.num_devices]);
			device->DEV_device_number = hysweeptmp.num_devices;
			device->DEV_device_capability = 4;
			strcpy(device->DEV_device_name, "MB-System interpolated position");
			device->DV2_device_capability = 0x0004;
			device->DV2_towfish = 0;
			device->DV2_enabled = 1;

			device->num_offsets = 3;
			for (i = 0; i < device->num_offsets; i++) {
				offset = (struct mbsys_hysweep_device_offset_struct *)&(device->offsets[i]);
				offset->OF2_device_number = hysweeptmp.num_devices;
				offset->OF2_offset_type = i;
				offset->OF2_offset_starboard = 0.0;
				offset->OF2_offset_forward = 0.0;
				offset->OF2_offset_vertical = 0.0;
				offset->OF2_offset_yaw = 0.0;
				offset->OF2_offset_roll = 0.0;
				offset->OF2_offset_pitch = 0.0;
				offset->OF2_offset_time = 0.0;
			}
			hysweeptmp.num_devices++;
		}
		if (*add_MB_GYR == MB_YES) {
			*device_number_MB_GYR = hysweeptmp.num_devices;
			device = (struct mbsys_hysweep_device_struct *)&(hysweeptmp.devices[hysweeptmp.num_devices]);
			device->DEV_device_number = hysweeptmp.num_devices;
			device->DEV_device_capability = 32;
			strcpy(device->DEV_device_name, "MB-System interpolated heading");
			device->DV2_device_capability = 0x0020;
			device->DV2_towfish = 0;
			device->DV2_enabled = 1;

			device->num_offsets = 2;
			for (i = 0; i < device->num_offsets; i++) {
				offset = (struct mbsys_hysweep_device_offset_struct *)&(device->offsets[i]);
				offset->OF2_device_number = hysweeptmp.num_devices;
				offset->OF2_offset_type = i + 1;
				offset->OF2_offset_starboard = 0.0;
				offset->OF2_offset_forward = 0.0;
				offset->OF2_offset_vertical = 0.0;
				offset->OF2_offset_yaw = 0.0;
				offset->OF2_offset_roll = 0.0;
				offset->OF2_offset_pitch = 0.0;
				offset->OF2_offset_time = 0.0;
			}
			hysweeptmp.num_devices++;
		}
		if (*add_MB_HCP == MB_YES) {
			*device_number_MB_HCP = hysweeptmp.num_devices;
			device = (struct mbsys_hysweep_device_struct *)&(hysweeptmp.devices[hysweeptmp.num_devices]);
			device->DEV_device_number = hysweeptmp.num_devices;
			device->DEV_device_capability = 512;
			strcpy(device->DEV_device_name, "MB-System interpolated attitude");
			device->DV2_device_capability = 0x0200;
			device->DV2_towfish = 0;
			device->DV2_enabled = 1;

			device->num_offsets = 2;
			for (i = 0; i < device->num_offsets; i++) {
				offset = (struct mbsys_hysweep_device_offset_struct *)&(device->offsets[i]);
				offset->OF2_device_number = hysweeptmp.num_devices;
				offset->OF2_offset_type = i + 1;
				offset->OF2_offset_starboard = 0.0;
				offset->OF2_offset_forward = 0.0;
				offset->OF2_offset_vertical = 0.0;
				offset->OF2_offset_yaw = 0.0;
				offset->OF2_offset_roll = 0.0;
				offset->OF2_offset_pitch = 0.0;
				offset->OF2_offset_time = 0.0;
			}
			hysweeptmp.num_devices++;
		}
		if (*add_MB_DFT == MB_YES) {
			*device_number_MB_DFT = hysweeptmp.num_devices;
			device = (struct mbsys_hysweep_device_struct *)&(hysweeptmp.devices[hysweeptmp.num_devices]);
			device->DEV_device_number = hysweeptmp.num_devices;
			device->DEV_device_capability = 16384;
			strcpy(device->DEV_device_name, "MB-System interpolated sonar depth");
			device->DV2_device_capability = 0x8000;
			device->DV2_towfish = 0;
			device->DV2_enabled = 1;

			device->num_offsets = 3;
			for (i = 0; i < device->num_offsets; i++) {
				offset = (struct mbsys_hysweep_device_offset_struct *)&(device->offsets[i]);
				offset->OF2_device_number = hysweeptmp.num_devices;
				offset->OF2_offset_type = i;
				offset->OF2_offset_starboard = 0.0;
				offset->OF2_offset_forward = 0.0;
				offset->OF2_offset_vertical = 0.0;
				offset->OF2_offset_yaw = 0.0;
				offset->OF2_offset_roll = 0.0;
				offset->OF2_offset_pitch = 0.0;
				offset->OF2_offset_time = 0.0;
			}
			hysweeptmp.num_devices++;
		}

		/* print debug statements */
		if (verbose >= 4) {
			fprintf(stderr, "\ndbg4  FTP data record to be written by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       FTP_record:                        %s\n", hysweeptmp.FTP_record);
			fprintf(stderr, "\ndbg4  HSX data record to be written by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       HSX_record:                        %d\n", hysweeptmp.HSX_record);
			fprintf(stderr, "\ndbg4  VER data record to be written by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       VER_version:                       %s\n", hysweeptmp.VER_version);
			fprintf(stderr, "\ndbg4  TND data record to be written by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       TND_survey_time_i[0]:              %d\n", hysweeptmp.TND_survey_time_i[0]);
			fprintf(stderr, "dbg4       TND_survey_time_i[1]:              %d\n", hysweeptmp.TND_survey_time_i[1]);
			fprintf(stderr, "dbg4       TND_survey_time_i[2]:              %d\n", hysweeptmp.TND_survey_time_i[2]);
			fprintf(stderr, "dbg4       TND_survey_time_i[3]:              %d\n", hysweeptmp.TND_survey_time_i[3]);
			fprintf(stderr, "dbg4       TND_survey_time_i[4]:              %d\n", hysweeptmp.TND_survey_time_i[4]);
			fprintf(stderr, "dbg4       TND_survey_time_i[5]:              %d\n", hysweeptmp.TND_survey_time_i[5]);
			fprintf(stderr, "dbg4       TND_survey_time_i[6]:              %d\n", hysweeptmp.TND_survey_time_i[6]);
			fprintf(stderr, "dbg4       TND_survey_time_d:                 %f\n", hysweeptmp.TND_survey_time_d);
			fprintf(stderr, "\ndbg4  INF data record to be written by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       INF_surveyor:                      %s\n", hysweeptmp.INF_surveyor);
			fprintf(stderr, "dbg4       INF_boat:                          %s\n", hysweeptmp.INF_boat);
			fprintf(stderr, "dbg4       INF_project:                       %s\n", hysweeptmp.INF_project);
			fprintf(stderr, "dbg4       INF_area:                          %s\n", hysweeptmp.INF_area);
			fprintf(stderr, "dbg4       INF_tide_correction:               %f\n", hysweeptmp.INF_tide_correction);
			fprintf(stderr, "dbg4       INF_draft_correction:              %f\n", hysweeptmp.INF_draft_correction);
			fprintf(stderr, "dbg4       INF_sound_velocity:                %f\n", hysweeptmp.INF_sound_velocity);
			fprintf(stderr, "\ndbg4  HSP data record to be written by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       HSP_minimum_depth:                 %f\n", hysweeptmp.HSP_minimum_depth);
			fprintf(stderr, "dbg4       HSP_maximum_depth:                 %f\n", hysweeptmp.HSP_maximum_depth);
			fprintf(stderr, "dbg4       HSP_port_offset_limit:             %f\n", hysweeptmp.HSP_port_offset_limit);
			fprintf(stderr, "dbg4       HSP_stbd_offset_limit:             %f\n", hysweeptmp.HSP_stbd_offset_limit);
			fprintf(stderr, "dbg4       HSP_port_angle_limit:              %f\n", hysweeptmp.HSP_port_angle_limit);
			fprintf(stderr, "dbg4       HSP_stbd_angle_limit:              %f\n", hysweeptmp.HSP_stbd_angle_limit);
			fprintf(stderr, "dbg4       HSP_high_beam_quality:             %d\n", hysweeptmp.HSP_high_beam_quality);
			fprintf(stderr, "dbg4       HSP_low_beam_quality:              %d\n", hysweeptmp.HSP_low_beam_quality);
			fprintf(stderr, "dbg4       HSP_sonar_range:                   %f\n", hysweeptmp.HSP_sonar_range);
			fprintf(stderr, "dbg4       HSP_towfish_layback:               %f\n", hysweeptmp.HSP_towfish_layback);
			fprintf(stderr, "dbg4       HSP_units:                         %d\n", hysweeptmp.HSP_units);
			fprintf(stderr, "dbg4       HSP_sonar_id:                      %d\n", hysweeptmp.HSP_sonar_id);
			fprintf(stderr, "\ndbg4  EOH data record to be written by MBIO function <%s>\n", function_name);
			fprintf(stderr, "\ndbg4  HVF data record to be written by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       HVF_time_after_midnight:           %f\n", hysweeptmp.HVF_time_after_midnight);
			fprintf(stderr, "dbg4       HVF_minimum_depth:                 %f\n", hysweeptmp.HVF_minimum_depth);
			fprintf(stderr, "dbg4       HVF_maximum_depth:                 %f\n", hysweeptmp.HVF_maximum_depth);
			fprintf(stderr, "dbg4       HVF_port_offset_limit:             %f\n", hysweeptmp.HVF_port_offset_limit);
			fprintf(stderr, "dbg4       HVF_starboard_offset_limit:        %f\n", hysweeptmp.HVF_starboard_offset_limit);
			fprintf(stderr, "dbg4       HVF_minimum_angle_limit:           %f\n", hysweeptmp.HVF_minimum_angle_limit);
			fprintf(stderr, "dbg4       HVF_maximum_angle_limit:           %f\n", hysweeptmp.HVF_maximum_angle_limit);
			fprintf(stderr, "\ndbg4  FIX data record to be written by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       FIX_device_number:                 %d\n", hysweeptmp.FIX_device_number);
			fprintf(stderr, "dbg4       FIX_time_after_midnight:           %f\n", hysweeptmp.FIX_time_after_midnight);
			fprintf(stderr, "dbg4       FIX_event_number:                  %d\n", hysweeptmp.FIX_event_number);
		}

		fprintf(mbfp, "FTP %s\r\n", hysweeptmp.FTP_record);
		fprintf(mbfp, "HSX %d\r\n", hysweeptmp.HSX_record);
		fprintf(mbfp, "VER %s\r\n", hysweeptmp.VER_version);
		fprintf(mbfp, "TND %2.2d:%2.2d:%2.2d %2.2d/%2.2d/%4.4d\r\n", hysweeptmp.TND_survey_time_i[3],
		        hysweeptmp.TND_survey_time_i[4], hysweeptmp.TND_survey_time_i[5], hysweeptmp.TND_survey_time_i[1],
		        hysweeptmp.TND_survey_time_i[2], hysweeptmp.TND_survey_time_i[0]);
		fprintf(mbfp, "INF \"%s\" \"%s\" \"%s\" \"%s\" %.2f %.2f %.2f\r\n", hysweeptmp.INF_surveyor, hysweeptmp.INF_boat,
		        hysweeptmp.INF_project, hysweeptmp.INF_area, hysweeptmp.INF_tide_correction, hysweeptmp.INF_draft_correction,
		        hysweeptmp.INF_sound_velocity);
		fprintf(mbfp, "HSP %.2f %.2f %.2f %.2f %g %g %d %d %.2f %.2f %d %d\r\n", hysweeptmp.HSP_minimum_depth,
		        hysweeptmp.HSP_maximum_depth, hysweeptmp.HSP_port_offset_limit, hysweeptmp.HSP_stbd_offset_limit,
		        hysweeptmp.HSP_port_angle_limit, hysweeptmp.HSP_stbd_angle_limit, hysweeptmp.HSP_high_beam_quality,
		        hysweeptmp.HSP_low_beam_quality, hysweeptmp.HSP_sonar_range, hysweeptmp.HSP_towfish_layback, hysweeptmp.HSP_units,
		        hysweeptmp.HSP_sonar_id);

		for (i = 0; i < hysweeptmp.num_devices; i++) {
			device = (struct mbsys_hysweep_device_struct *)&(hysweeptmp.devices[i]);
			fprintf(mbfp, "DEV %d %d \"%s\"\r\n", device->DEV_device_number, device->DEV_device_capability,
			        device->DEV_device_name);
			fprintf(mbfp, "DV2 %d %x %d %d\r\n", device->DEV_device_number, device->DV2_device_capability, device->DV2_towfish,
			        device->DV2_enabled);
			for (j = 0; j < device->num_offsets; j++) {
				offset = (struct mbsys_hysweep_device_offset_struct *)&(device->offsets[j]);
				fprintf(mbfp, "OF2 %d %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f\r\n", offset->OF2_device_number,
				        offset->OF2_offset_type, offset->OF2_offset_starboard, offset->OF2_offset_forward,
				        offset->OF2_offset_vertical, offset->OF2_offset_yaw, offset->OF2_offset_roll, offset->OF2_offset_pitch,
				        offset->OF2_offset_time);
			}
			if (device->PRI_primary_nav_device == i) {
				fprintf(mbfp, "PRI 1\r\n");
			}
			if (device->DV2_device_capability & 1 || device->DV2_device_capability & 2) {
				fprintf(mbfp, "MBI %d %x %x %x %d %d %.3f %.3f\r\n", device->DEV_device_number, device->MBI_sonar_type,
				        device->MBI_sonar_flags, device->MBI_beam_data_available, device->MBI_num_beams_1,
				        device->MBI_num_beams_2, device->MBI_first_beam_angle, device->MBI_angle_increment);
				fprintf(mbfp, "SSI %d %x %d %d\r\n", device->DEV_device_number, device->SSI_sonar_flags,
				        device->SSI_port_num_samples, device->SSI_starboard_num_samples);
			}
		}

		fprintf(mbfp, "PRJ %s\r\n", hysweeptmp.PRJ_proj4_command);
		fprintf(mbfp, "EOH\r\n");
		fprintf(mbfp, "HVF 99 %.3f %.1f %.1f %.1f %.1f %.1f %.1f\r\n", hysweeptmp.HVF_time_after_midnight,
		        hysweeptmp.HVF_minimum_depth, hysweeptmp.HVF_maximum_depth, hysweeptmp.HVF_port_offset_limit,
		        hysweeptmp.HVF_starboard_offset_limit, hysweeptmp.HVF_minimum_angle_limit, hysweeptmp.HVF_maximum_angle_limit);
		fprintf(mbfp, "FIX %d %.3f %d\r\n", hysweeptmp.FIX_device_number, hysweeptmp.FIX_time_after_midnight,
		        hysweeptmp.FIX_event_number);
		*file_header_written = MB_YES;

		/* initialize projection */
		mb_proj_init(verbose, store->PRJ_proj4_command, &(mb_io_ptr->pjptr), error);
		mb_io_ptr->projection_initialized = MB_YES;
	}

	/* call appropriate writing routines for ping data */
	if (status == MB_SUCCESS && store->kind == MB_DATA_DATA) {
		/* print debug statements */
		if (verbose >= 4) {
			fprintf(stderr, "\ndbg4  RMB data record read by MBIO function <%s>\n", function_name);
			fprintf(stderr, "dbg4       RMB_device_number:                 %d\n", store->RMB_device_number);
			fprintf(stderr, "dbg4       RMB_time:                          %f\n", store->RMB_time);
			fprintf(stderr, "dbg4       RMB_sonar_type:                    %x\n", store->RMB_sonar_type);
			fprintf(stderr, "dbg4       RMB_sonar_flags:                   %x\n", store->RMB_sonar_flags);
			fprintf(stderr, "dbg4       RMB_beam_data_available:           %x\n", store->RMB_beam_data_available);
			fprintf(stderr, "dbg4       RMB_num_beams:                     %d\n", store->RMB_num_beams);
			fprintf(stderr, "dbg4       RMB_num_beams_alloc:               %d\n", store->RMB_num_beams_alloc);
			fprintf(stderr, "dbg4       RMB_sound_velocity:                %f\n", store->RMB_sound_velocity);
			fprintf(stderr, "dbg4       RMB_ping_number:                   %d\n", store->RMB_ping_number);
			for (i = 0; i < store->RMB_num_beams; i++) {
				fprintf(stderr, "dbg4       beam:%4d", i);

				if (store->RMB_beam_data_available & 0x0001)
					fprintf(stderr, " %f", store->RMB_beam_ranges[i]);

				if (store->RMB_beam_data_available & 0x0002)
					fprintf(stderr, " %f", store->RMB_multi_ranges[i]);

				if (store->RMB_beam_data_available & 0x0004)
					fprintf(stderr, " %f", store->RMB_sounding_eastings[i]);

				if (store->RMB_beam_data_available & 0x0004)
					fprintf(stderr, " %f", store->RMB_sounding_northings[i]);

				if (store->RMB_beam_data_available & 0x0008)
					fprintf(stderr, " %f", store->RMB_sounding_depths[i]);

				if (store->RMB_beam_data_available & 0x0010)
					fprintf(stderr, " %f", store->RMB_sounding_along[i]);

				if (store->RMB_beam_data_available & 0x0020)
					fprintf(stderr, " %f", store->RMB_sounding_across[i]);

				if (store->RMB_beam_data_available & 0x0040)
					fprintf(stderr, " %f", store->RMB_sounding_pitchangles[i]);

				if (store->RMB_beam_data_available & 0x0080)
					fprintf(stderr, " %f", store->RMB_sounding_rollangles[i]);

				if (store->RMB_beam_data_available & 0x0100)
					fprintf(stderr, " %f", store->RMB_sounding_takeoffangles[i]);

				if (store->RMB_beam_data_available & 0x0200)
					fprintf(stderr, " %f", store->RMB_sounding_azimuthalangles[i]);

				if (store->RMB_beam_data_available & 0x0400)
					fprintf(stderr, " %d", store->RMB_sounding_timedelays[i]);

				if (store->RMB_beam_data_available & 0x0800)
					fprintf(stderr, " %d", store->RMB_sounding_intensities[i]);

				if (store->RMB_beam_data_available & 0x1000)
					fprintf(stderr, " %d", store->RMB_sounding_quality[i]);

				if (store->RMB_beam_data_available & 0x2000)
					fprintf(stderr, " %d", store->RMB_sounding_flags[i]);

				fprintf(stderr, "\n");
			}
		}

		/* add desired interpolated data */
		if (*add_MB_POS == MB_YES) {
			sprintf(line, "POS %d %.3f %.2f %.2f\r\n", *device_number_MB_POS, store->RMB_time, store->RMBint_x, store->RMBint_y);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}
		if (*add_MB_GYR == MB_YES) {
			sprintf(line, "GYR %d %.3f %.2f\r\n", *device_number_MB_GYR, store->RMB_time, store->RMBint_heading);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}
		if (*add_MB_HCP == MB_YES) {
			sprintf(line, "HCP %d %.3f %.2f %.2f %.2f\r\n", *device_number_MB_HCP, store->RMB_time, (store->RMBint_heave),
			        (-store->RMBint_roll), store->RMBint_pitch);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}
		if (*add_MB_DFT == MB_YES) {
			sprintf(line, "DFT %d %.3f %.2f\r\n", *device_number_MB_DFT, store->RMB_time, store->RMBint_draft);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}

		/* write raw sidescan if it exists */
		if ((store->RSS_ping_number == store->RMB_ping_number || 10 * store->RSS_ping_number == store->RMB_ping_number) &&
		    store->RSS_port_num_samples + store->RSS_starboard_num_samples > 0) {
			/* write first line */
			sprintf(line, "RSS %d %.3f %x %d %d %.2f %d %.2f %f %d %d %d %d\r\n", store->RSS_device_number, store->RSS_time,
			        store->RSS_sonar_flags, store->RSS_port_num_samples, store->RSS_starboard_num_samples,
			        store->RSS_sound_velocity, store->RSS_ping_number, store->RSS_altitude, store->RSS_sample_rate,
			        store->RSS_minimum_amplitude, store->RSS_maximum_amplitude, store->RSS_bit_shift, store->RSS_frequency);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */

			/* write RSS_port */
			for (i = 0; i < store->RSS_port_num_samples; i++) {
				if (i != 0)
					fprintf(mbfp, " ");
				fprintf(mbfp, "%d", store->RSS_port[i]);
			}
			fprintf(mbfp, "\r\n");

			/* write RSS_starboard */
			for (i = 0; i < store->RSS_starboard_num_samples; i++) {
				if (i != 0)
					fprintf(mbfp, " ");
				fprintf(mbfp, "%d", store->RSS_starboard[i]);
			}
			fprintf(mbfp, "\r\n");
		}

		/* write processed sidescan if it exists */
		/* fprintf(stderr,"store->MSS_num_pixels:%d store->MSS_ping_number:%d store->RMB_ping_number:%d\n",
		store->MSS_num_pixels,store->MSS_ping_number,store->RMB_ping_number); */
		if ((store->MSS_ping_number == store->RMB_ping_number || 10 * store->MSS_ping_number == store->RMB_ping_number) &&
		    store->MSS_num_pixels > 0) {
			/* write first line */
			sprintf(line, "MSS %d %.3f %.2f %d %.3f %d\r\n", store->MSS_device_number, store->MSS_time, store->MSS_sound_velocity,
			        store->MSS_num_pixels, store->MSS_pixel_size, store->MSS_ping_number);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */

			/* write MSS_ss */
			for (i = 0; i < store->MSS_num_pixels; i++) {
				if (i != 0)
					fprintf(mbfp, " ");
				fprintf(mbfp, "%.2f", store->MSS_ss[i]);
			}
			fprintf(mbfp, "\r\n");

			/* write MSS_ss_along */
			for (i = 0; i < store->MSS_num_pixels; i++) {
				if (i != 0)
					fprintf(mbfp, " ");
				fprintf(mbfp, "%.2f", store->MSS_ss_along[i]);
			}
			fprintf(mbfp, "\r\n");
		}

		/* write SNR record if it exists */
		if ((store->SNR_ping_number == store->RMB_ping_number || 10 * store->SNR_ping_number == store->RMB_ping_number)) {
			sprintf(line, "SNR %d %.3f %d %d %d", store->SNR_device_number, store->SNR_time, store->SNR_ping_number,
			        store->SNR_sonar_id, store->SNR_num_settings);
			for (i = 0; i < store->SNR_num_settings; i++) {
				kindex = strlen(line);
				sprintf(line + kindex, " %g", store->SNR_settings[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}

		/* write first line */
		sprintf(line, "RMB %d %.3f %x %x %x %d %.2f %d\r\n", store->RMB_device_number, store->RMB_time, store->RMB_sonar_type,
		        store->RMB_sonar_flags, store->RMB_beam_data_available, store->RMB_num_beams, store->RMB_sound_velocity,
		        store->RMB_ping_number);
		fputs(line, mbfp);
		/* fprintf(stderr,"writeline: %s",line); */

		/* write RMB_beam_ranges if included */
		if (store->RMB_beam_data_available & 0x0001) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_beam_ranges[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_beam_ranges[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_multi_ranges if included */
		if (store->RMB_beam_data_available & 0x0002) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_multi_ranges[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_multi_ranges[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_eastings if included */
		if (store->RMB_beam_data_available & 0x0004) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_sounding_eastings[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_sounding_eastings[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_northings if included */
		if (store->RMB_beam_data_available & 0x0004) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_sounding_northings[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_sounding_northings[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_depths if included */
		if (store->RMB_beam_data_available & 0x0008) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_sounding_depths[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_sounding_depths[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_along if included */
		if (store->RMB_beam_data_available & 0x0010) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_sounding_along[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_sounding_along[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_across if included */
		if (store->RMB_beam_data_available & 0x0020) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_sounding_across[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_sounding_across[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_pitchangles if included */
		if (store->RMB_beam_data_available & 0x0040) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_sounding_pitchangles[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_sounding_pitchangles[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_rollangles if included */
		if (store->RMB_beam_data_available & 0x0080) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_sounding_rollangles[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_sounding_rollangles[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_takeoffangles if included */
		if (store->RMB_beam_data_available & 0x0100) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_sounding_takeoffangles[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_sounding_takeoffangles[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_azimuthalangles if included */
		if (store->RMB_beam_data_available & 0x0200) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %.2f", store->RMB_sounding_azimuthalangles[i]);
				else
					sprintf(line + kindex, "%.2f", store->RMB_sounding_azimuthalangles[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_timedelays if included */
		if (store->RMB_beam_data_available & 0x0400) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %d", store->RMB_sounding_timedelays[i]);
				else
					sprintf(line + kindex, "%d", store->RMB_sounding_timedelays[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_intensities if included */
		if (store->RMB_beam_data_available & 0x0800) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %d", store->RMB_sounding_intensities[i]);
				else
					sprintf(line + kindex, "%d", store->RMB_sounding_intensities[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_quality if included */
		if (store->RMB_beam_data_available & 0x1000) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %d", store->RMB_sounding_quality[i]);
				else
					sprintf(line + kindex, "%d", store->RMB_sounding_quality[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}

		/* write RMB_sounding_flags if included */
		if (store->RMB_beam_data_available & 0x2000) {
			/* write the next line */
			line[0] = '\0';
			for (i = 0; i < store->RMB_num_beams; i++) {
				kindex = strlen(line);
				if (i > 0)
					sprintf(line + kindex, " %d", store->RMB_sounding_flags[i]);
				else
					sprintf(line + kindex, "%d", store->RMB_sounding_flags[i]);
			}
			kindex = strlen(line);
			sprintf(line + kindex, "\r\n");
			fputs(line, mbfp);
		}
	}

	/* call appropriate writing routine for other records */
	else if (status == MB_SUCCESS) {
		/* write HCP record */
		if (store->kind == MB_DATA_ATTITUDE) {
			sprintf(line, "HCP %d %.3f %.2f %.2f %.2f\r\n", store->HCP_device_number, store->HCP_time, store->HCP_heave,
			        store->HCP_roll, store->HCP_pitch);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}

		/* write GYR record */
		else if (store->kind == MB_DATA_HEADING) {
			sprintf(line, "GYR %d %.3f %.2f\r\n", store->GYR_device_number, store->GYR_time, store->GYR_heading);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}

		/* write DFT record */
		else if (store->kind == MB_DATA_SONARDEPTH) {
			sprintf(line, "DFT %d %.3f %.2f\r\n", store->DFT_device_number, store->DFT_time, store->DFT_draft);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}

		/* write GPS and POS record */
		else if (store->kind == MB_DATA_NAV || store->kind == MB_DATA_NAV1) {
			if (store->GPS_device_number == store->POS_device_number) {
				sprintf(line, "GPS %d %.3f %.2f %.2f %.2f %d %d\r\n", store->GPS_device_number, store->GPS_time, store->GPS_cog,
				        store->GPS_sog, store->GPS_hdop, store->GPS_mode, store->GPS_nsats);
				fputs(line, mbfp);
				/* fprintf(stderr,"writeline: %s",line); */
			}

			sprintf(line, "POS %d %.3f %.2f %.2f\r\n", store->POS_device_number, store->POS_time, store->POS_x, store->POS_y);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}

		/* write EC1 record */
		else if (store->kind == MB_DATA_ALTITUDE) {
			sprintf(line, "EC1 %d %.3f %.2f\r\n", store->EC1_device_number, store->EC1_time, store->EC1_rawdepth);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}

		/* write TID record */
		else if (store->kind == MB_DATA_TIDE) {
			sprintf(line, "TID %d %.3f %.2f\r\n", store->TID_device_number, store->TID_time, store->TID_tide);
			fputs(line, mbfp);
			/* fprintf(stderr,"writeline: %s",line); */
		}

		/* write comment */
		else if (store->kind == MB_DATA_COMMENT) {
			sprintf(line, "COM %s\r\n", store->COM_comment);
			/* fprintf(stderr,"writeline: %s",line); */
		}
	}

#ifdef MBR_HYSWEEP1_DEBUG
	fprintf(stderr, "HYSWEEP1 DATA WRITTEN: type:%d status:%d error:%d\n\n", store->kind, status, *error);
#endif

	/* print output debug statements */
	if (verbose >= 2) {
		fprintf(stderr, "\ndbg2  MBIO function <%s> completed\n", function_name);
		fprintf(stderr, "dbg2  Return values:\n");
		fprintf(stderr, "dbg2       error:      %d\n", *error);
		fprintf(stderr, "dbg2  Return status:\n");
		fprintf(stderr, "dbg2       status:  %d\n", status);
	}

	/* return status */
	return (status);
}
/*--------------------------------------------------------------------*/
