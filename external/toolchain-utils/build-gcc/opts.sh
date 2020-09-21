get_gcc_configure_options()
{
  local CTARGET=$1; shift
  local confgcc=$(get_gcc_common_options)
  case ${CTARGET} in
    arm*)	#264534
      local arm_arch="${CTARGET%%-*}"
      # Only do this if arm_arch is armv*
      if [[ ${arm_arch} == armv* ]] ; then
        # Convert armv7{a,r,m} to armv7-{a,r,m}
        [[ ${arm_arch} == armv7? ]] && arm_arch=${arm_arch/7/7-}
        # Remove endian ('l' / 'eb')
        [[ ${arm_arch} == *l  ]] && arm_arch=${arm_arch%l}
        [[ ${arm_arch} == *eb ]] && arm_arch=${arm_arch%eb}
        confgcc="${confgcc} --with-arch=${arm_arch}"
        confgcc="${confgcc} --disable-esp"
      fi
      ;;
    i?86*)
      # Hardened is enabled for x86, but disabled for ARM.
      confgcc="${confgcc} --with-arch=atom"
      confgcc="${confgcc} --enable-esp"
      ;;
  esac
  echo ${confgcc}
}

get_gcc_common_options()
{
  local confgcc
  # TODO(asharif): Build without these options.
  confgcc="${confgcc} --disable-libmudflap"
  confgcc="${confgcc} --disable-libssp"
  confgcc="${confgcc} --disable-libgomp"
  confgcc="${confgcc} --enable-__cxa_atexit"
  confgcc="${confgcc} --enable-checking=release"
  confgcc="${confgcc} --disable-libquadmath"
  echo ${confgcc}
}

