Name:		zvbi
Summary:	Raw VBI, Teletext and Closed Caption decoding library
Version:	0.2.33
Release:	1
Copyright:	GPL
Group:		Applications/Multimedia
Url:		http://zapping.sourceforge.net/
Source:		http://prdownloads.sourceforge.net/zapping/%{name}-%{version}.tar.bz2
Packager:	Michael H. Schimek <mschimek@users.sourceforge.net>
Buildroot:	%{_tmppath}/%{name}-%{version}-root
PreReq: 	/sbin/install-info
Provides:	zvbi

%description
Routines to access raw VBI capture devices (currently the V4L and
V4L2 API, and the *BSD bktr driver are supported), a versatile VBI
bit slicer, decoders for various data services and basic search,
render and export functions for Closed Caption and Teletext pages.

%prep
%setup -q

%build
%configure
make

%install
rm -rf %{buildroot}
%makeinstall
%find_lang %{name}

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr (-, root, root)
%doc AUTHORS BUGS COPYING ChangeLog NEWS README TODO doc/html
%{_libdir}/libzvbi*
%{_includedir}/libzvbi.h

%changelog
* Thu Oct 24 2002 Michael H. Schimek <mschimek@users.sourceforge.net>
- Added %%find_lang for locale support.
