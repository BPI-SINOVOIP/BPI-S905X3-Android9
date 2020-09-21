#ifndef _GPXE_ERRFILE_H
#define _GPXE_ERRFILE_H

/** @file
 *
 * Error file identifiers
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <bits/errfile.h>

/**
 * @defgroup errfilecat Error file identifier categories
 *
 * @{
 */

#define ERRFILE_CORE		0x00002000	/**< Core code */
#define ERRFILE_DRIVER		0x00004000	/**< Driver code */
#define ERRFILE_NET		0x00006000	/**< Networking code */
#define ERRFILE_IMAGE		0x00008000	/**< Image code */
#define ERRFILE_OTHER		0x0000e000	/**< Any other code */

/** @} */

/** Flag for architecture-dependent error files */
#define ERRFILE_ARCH		0x00800000

/**
 * @defgroup errfile Error file identifiers
 *
 * These values are automatically incorporated into the definitions
 * for error numbers such as EINVAL.
 *
 * @{
 */

#define ERRFILE_asprintf	       ( ERRFILE_CORE | 0x00000000 )
#define ERRFILE_downloader	       ( ERRFILE_CORE | 0x00010000 )
#define ERRFILE_exec		       ( ERRFILE_CORE | 0x00020000 )
#define ERRFILE_hw		       ( ERRFILE_CORE | 0x00030000 )
#define ERRFILE_iobuf		       ( ERRFILE_CORE | 0x00040000 )
#define ERRFILE_job		       ( ERRFILE_CORE | 0x00050000 )
#define ERRFILE_linebuf		       ( ERRFILE_CORE | 0x00060000 )
#define ERRFILE_monojob		       ( ERRFILE_CORE | 0x00070000 )
#define ERRFILE_nvo		       ( ERRFILE_CORE | 0x00080000 )
#define ERRFILE_open		       ( ERRFILE_CORE | 0x00090000 )
#define ERRFILE_posix_io	       ( ERRFILE_CORE | 0x000a0000 )
#define ERRFILE_resolv		       ( ERRFILE_CORE | 0x000b0000 )
#define ERRFILE_settings	       ( ERRFILE_CORE | 0x000c0000 )
#define ERRFILE_vsprintf	       ( ERRFILE_CORE | 0x000d0000 )
#define ERRFILE_xfer		       ( ERRFILE_CORE | 0x000e0000 )
#define ERRFILE_bitmap		       ( ERRFILE_CORE | 0x000f0000 )

#define ERRFILE_eisa		     ( ERRFILE_DRIVER | 0x00000000 )
#define ERRFILE_isa		     ( ERRFILE_DRIVER | 0x00010000 )
#define ERRFILE_isapnp		     ( ERRFILE_DRIVER | 0x00020000 )
#define ERRFILE_mca		     ( ERRFILE_DRIVER | 0x00030000 )
#define ERRFILE_pci		     ( ERRFILE_DRIVER | 0x00040000 )

#define ERRFILE_nvs		     ( ERRFILE_DRIVER | 0x00100000 )
#define ERRFILE_spi		     ( ERRFILE_DRIVER | 0x00110000 )
#define ERRFILE_i2c_bit		     ( ERRFILE_DRIVER | 0x00120000 )
#define ERRFILE_spi_bit		     ( ERRFILE_DRIVER | 0x00130000 )

#define ERRFILE_3c509		     ( ERRFILE_DRIVER | 0x00200000 )
#define ERRFILE_bnx2		     ( ERRFILE_DRIVER | 0x00210000 )
#define ERRFILE_cs89x0		     ( ERRFILE_DRIVER | 0x00220000 )
#define ERRFILE_eepro		     ( ERRFILE_DRIVER | 0x00230000 )
#define ERRFILE_etherfabric	     ( ERRFILE_DRIVER | 0x00240000 )
#define ERRFILE_legacy		     ( ERRFILE_DRIVER | 0x00250000 )
#define ERRFILE_natsemi		     ( ERRFILE_DRIVER | 0x00260000 )
#define ERRFILE_pnic		     ( ERRFILE_DRIVER | 0x00270000 )
#define ERRFILE_prism2_pci	     ( ERRFILE_DRIVER | 0x00280000 )
#define ERRFILE_prism2_plx	     ( ERRFILE_DRIVER | 0x00290000 )
#define ERRFILE_rtl8139		     ( ERRFILE_DRIVER | 0x002a0000 )
#define ERRFILE_smc9000		     ( ERRFILE_DRIVER | 0x002b0000 )
#define ERRFILE_tg3		     ( ERRFILE_DRIVER | 0x002c0000 )
#define ERRFILE_3c509_eisa	     ( ERRFILE_DRIVER | 0x002d0000 )
#define ERRFILE_3c515		     ( ERRFILE_DRIVER | 0x002e0000 )
#define ERRFILE_3c529		     ( ERRFILE_DRIVER | 0x002f0000 )
#define ERRFILE_3c595		     ( ERRFILE_DRIVER | 0x00300000 )
#define ERRFILE_3c5x9		     ( ERRFILE_DRIVER | 0x00310000 )
#define ERRFILE_3c90x		     ( ERRFILE_DRIVER | 0x00320000 )
#define ERRFILE_amd8111e	     ( ERRFILE_DRIVER | 0x00330000 )
#define ERRFILE_davicom		     ( ERRFILE_DRIVER | 0x00340000 )
#define ERRFILE_depca		     ( ERRFILE_DRIVER | 0x00350000 )
#define ERRFILE_dmfe		     ( ERRFILE_DRIVER | 0x00360000 )
#define ERRFILE_eepro100	     ( ERRFILE_DRIVER | 0x00380000 )
#define ERRFILE_epic100		     ( ERRFILE_DRIVER | 0x00390000 )
#define ERRFILE_forcedeth	     ( ERRFILE_DRIVER | 0x003a0000 )
#define ERRFILE_mtd80x		     ( ERRFILE_DRIVER | 0x003b0000 )
#define ERRFILE_ns83820		     ( ERRFILE_DRIVER | 0x003c0000 )
#define ERRFILE_ns8390		     ( ERRFILE_DRIVER | 0x003d0000 )
#define ERRFILE_pcnet32		     ( ERRFILE_DRIVER | 0x003e0000 )
#define ERRFILE_r8169		     ( ERRFILE_DRIVER | 0x003f0000 )
#define ERRFILE_sis900		     ( ERRFILE_DRIVER | 0x00400000 )
#define ERRFILE_sundance	     ( ERRFILE_DRIVER | 0x00410000 )
#define ERRFILE_tlan		     ( ERRFILE_DRIVER | 0x00420000 )
#define ERRFILE_tulip		     ( ERRFILE_DRIVER | 0x00430000 )
#define ERRFILE_via_rhine	     ( ERRFILE_DRIVER | 0x00440000 )
#define ERRFILE_via_velocity	     ( ERRFILE_DRIVER | 0x00450000 )
#define ERRFILE_w89c840		     ( ERRFILE_DRIVER | 0x00460000 )
#define ERRFILE_ipoib		     ( ERRFILE_DRIVER | 0x00470000 )
#define ERRFILE_e1000		     ( ERRFILE_DRIVER | 0x00480000 )
#define ERRFILE_e1000_hw	     ( ERRFILE_DRIVER | 0x00490000 )
#define ERRFILE_mtnic		     ( ERRFILE_DRIVER | 0x004a0000 )
#define ERRFILE_phantom		     ( ERRFILE_DRIVER | 0x004b0000 )
#define ERRFILE_ne2k_isa	     ( ERRFILE_DRIVER | 0x004c0000 )
#define ERRFILE_b44		     ( ERRFILE_DRIVER | 0x004d0000 )
#define ERRFILE_rtl818x		     ( ERRFILE_DRIVER | 0x004e0000 )
#define ERRFILE_sky2                 ( ERRFILE_DRIVER | 0x004f0000 )
#define ERRFILE_ath5k		     ( ERRFILE_DRIVER | 0x00500000 )
#define ERRFILE_atl1e		     ( ERRFILE_DRIVER | 0x00510000 )
#define ERRFILE_sis190		     ( ERRFILE_DRIVER | 0x00520000 )
#define ERRFILE_myri10ge	     ( ERRFILE_DRIVER | 0x00530000 )
#define ERRFILE_skge		     ( ERRFILE_DRIVER | 0x00540000 )

#define ERRFILE_scsi		     ( ERRFILE_DRIVER | 0x00700000 )
#define ERRFILE_arbel		     ( ERRFILE_DRIVER | 0x00710000 )
#define ERRFILE_hermon		     ( ERRFILE_DRIVER | 0x00720000 )
#define ERRFILE_linda		     ( ERRFILE_DRIVER | 0x00730000 )
#define ERRFILE_ata		     ( ERRFILE_DRIVER | 0x00740000 )
#define ERRFILE_srp		     ( ERRFILE_DRIVER | 0x00750000 )

#define ERRFILE_aoe			( ERRFILE_NET | 0x00000000 )
#define ERRFILE_arp			( ERRFILE_NET | 0x00010000 )
#define ERRFILE_dhcpopts		( ERRFILE_NET | 0x00020000 )
#define ERRFILE_ethernet		( ERRFILE_NET | 0x00030000 )
#define ERRFILE_icmpv6			( ERRFILE_NET | 0x00040000 )
#define ERRFILE_ipv4			( ERRFILE_NET | 0x00050000 )
#define ERRFILE_ipv6			( ERRFILE_NET | 0x00060000 )
#define ERRFILE_ndp			( ERRFILE_NET | 0x00070000 )
#define ERRFILE_netdevice		( ERRFILE_NET | 0x00080000 )
#define ERRFILE_nullnet			( ERRFILE_NET | 0x00090000 )
#define ERRFILE_tcp			( ERRFILE_NET | 0x000a0000 )
#define ERRFILE_ftp			( ERRFILE_NET | 0x000b0000 )
#define ERRFILE_http			( ERRFILE_NET | 0x000c0000 )
#define ERRFILE_iscsi			( ERRFILE_NET | 0x000d0000 )
#define ERRFILE_tcpip			( ERRFILE_NET | 0x000e0000 )
#define ERRFILE_udp			( ERRFILE_NET | 0x000f0000 )
#define ERRFILE_dhcp			( ERRFILE_NET | 0x00100000 )
#define ERRFILE_dns			( ERRFILE_NET | 0x00110000 )
#define ERRFILE_tftp			( ERRFILE_NET | 0x00120000 )
#define ERRFILE_infiniband		( ERRFILE_NET | 0x00130000 )
#define ERRFILE_netdev_settings		( ERRFILE_NET | 0x00140000 )
#define ERRFILE_dhcppkt			( ERRFILE_NET | 0x00150000 )
#define ERRFILE_slam			( ERRFILE_NET | 0x00160000 )
#define ERRFILE_ib_sma			( ERRFILE_NET | 0x00170000 )
#define ERRFILE_ib_packet		( ERRFILE_NET | 0x00180000 )
#define ERRFILE_icmp			( ERRFILE_NET | 0x00190000 )
#define ERRFILE_ib_qset			( ERRFILE_NET | 0x001a0000 )
#define ERRFILE_ib_gma			( ERRFILE_NET | 0x001b0000 )
#define ERRFILE_ib_pathrec		( ERRFILE_NET | 0x001c0000 )
#define ERRFILE_ib_mcast		( ERRFILE_NET | 0x001d0000 )
#define ERRFILE_ib_cm			( ERRFILE_NET | 0x001e0000 )
#define ERRFILE_net80211		( ERRFILE_NET | 0x001f0000 )
#define ERRFILE_ib_mi			( ERRFILE_NET | 0x00200000 )
#define ERRFILE_ib_cmrc			( ERRFILE_NET | 0x00210000 )
#define ERRFILE_ib_srp			( ERRFILE_NET | 0x00220000 )
#define ERRFILE_sec80211		( ERRFILE_NET | 0x00230000 )
#define ERRFILE_wep			( ERRFILE_NET | 0x00240000 )
#define ERRFILE_eapol			( ERRFILE_NET | 0x00250000 )
#define ERRFILE_wpa			( ERRFILE_NET | 0x00260000 )
#define ERRFILE_wpa_psk			( ERRFILE_NET | 0x00270000 )
#define ERRFILE_wpa_tkip		( ERRFILE_NET | 0x00280000 )
#define ERRFILE_wpa_ccmp		( ERRFILE_NET | 0x00290000 )

#define ERRFILE_image		      ( ERRFILE_IMAGE | 0x00000000 )
#define ERRFILE_elf		      ( ERRFILE_IMAGE | 0x00010000 )
#define ERRFILE_script		      ( ERRFILE_IMAGE | 0x00020000 )
#define ERRFILE_segment		      ( ERRFILE_IMAGE | 0x00030000 )
#define ERRFILE_efi_image	      ( ERRFILE_IMAGE | 0x00040000 )
#define ERRFILE_embedded	      ( ERRFILE_IMAGE | 0x00050000 )

#define ERRFILE_asn1		      ( ERRFILE_OTHER | 0x00000000 )
#define ERRFILE_chap		      ( ERRFILE_OTHER | 0x00010000 )
#define ERRFILE_aoeboot		      ( ERRFILE_OTHER | 0x00020000 )
#define ERRFILE_autoboot	      ( ERRFILE_OTHER | 0x00030000 )
#define ERRFILE_dhcpmgmt	      ( ERRFILE_OTHER | 0x00040000 )
#define ERRFILE_imgmgmt		      ( ERRFILE_OTHER | 0x00050000 )
#define ERRFILE_pxe_tftp	      ( ERRFILE_OTHER | 0x00060000 )
#define ERRFILE_pxe_udp		      ( ERRFILE_OTHER | 0x00070000 )
#define ERRFILE_axtls_aes	      ( ERRFILE_OTHER | 0x00080000 )
#define ERRFILE_cipher		      ( ERRFILE_OTHER | 0x00090000 )
#define ERRFILE_image_cmd	      ( ERRFILE_OTHER | 0x000a0000 )
#define ERRFILE_uri_test	      ( ERRFILE_OTHER | 0x000b0000 )
#define ERRFILE_ibft		      ( ERRFILE_OTHER | 0x000c0000 )
#define ERRFILE_tls		      ( ERRFILE_OTHER | 0x000d0000 )
#define ERRFILE_ifmgmt		      ( ERRFILE_OTHER | 0x000e0000 )
#define ERRFILE_iscsiboot	      ( ERRFILE_OTHER | 0x000f0000 )
#define ERRFILE_efi_pci		      ( ERRFILE_OTHER | 0x00100000 )
#define ERRFILE_efi_snp		      ( ERRFILE_OTHER | 0x00110000 )
#define ERRFILE_smbios		      ( ERRFILE_OTHER | 0x00120000 )
#define ERRFILE_smbios_settings	      ( ERRFILE_OTHER | 0x00130000 )
#define ERRFILE_efi_smbios	      ( ERRFILE_OTHER | 0x00140000 )
#define ERRFILE_pxemenu		      ( ERRFILE_OTHER | 0x00150000 )
#define ERRFILE_x509		      ( ERRFILE_OTHER | 0x00160000 )
#define ERRFILE_login_ui	      ( ERRFILE_OTHER | 0x00170000 )
#define ERRFILE_ib_srpboot	      ( ERRFILE_OTHER | 0x00180000 )
#define ERRFILE_iwmgmt		      ( ERRFILE_OTHER | 0x00190000 )

/** @} */

#endif /* _GPXE_ERRFILE_H */
