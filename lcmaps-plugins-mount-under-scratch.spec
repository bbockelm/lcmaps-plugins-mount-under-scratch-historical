Summary: LCMAPS plugin for creating scratch directories for glexec
Name: lcmaps-plugins-mount-under-scratch
Version: 0.0.1
Release: 0%{?dist}
License: Public Domain
Group: System Environment/Libraries

# The tarball was created from Subversion using the following commands:
# svn co svn://t2.unl.edu/brian/lcmaps-plugin-mount-under-scratch
# cd lcmaps-plugin-mount-under-scratch
# ./bootstrap
# ./configure
# make dist
Source0: %{name}-%{version}.tar.gz

BuildRequires: boost-devel
BuildRequires: lcmaps-interface

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
This plugin creates temporary directories for the payload process,
giving it a unique /tmp and /var/tmp

%prep
%setup -q

%build

%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install
rm $RPM_BUILD_ROOT%{_libdir}/lcmaps/liblcmaps_mount_under_scratch.la
rm $RPM_BUILD_ROOT%{_libdir}/lcmaps/liblcmaps_mount_under_scratch.a
mv $RPM_BUILD_ROOT%{_libdir}/lcmaps/liblcmaps_mount_under_scratch.so \
   $RPM_BUILD_ROOT%{_libdir}/lcmaps/lcmaps_mount_under_scratch.mod

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_libdir}/lcmaps/lcmaps_mount_under_scratch.mod

%changelog
