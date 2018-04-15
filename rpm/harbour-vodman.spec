%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

Name:       harbour-vodman
Summary:    Video On Demand (VOD) download manager for SailfishOS
Version:    1.0.6
Release:    1
Group:      Applications/Multimedia
#Group:      Qt/Qt
License:    MIT
#URL:        http://foo.bar
Source0:    %{name}-%{version}.tar.bz2

BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(sailfishapp) >= 0.0.10
BuildRequires:  desktop-file-utils


#Requires:   python3-base
Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   libvodman = %{version}
Requires:   vodman-service = %{version}

%description
%{summary}


%files
%defattr(-,root,root,-)
%attr(0755,root,root) %{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/%{name}/qml/*
%{_datadir}/%{name}/icons/*
%{_datadir}/%{name}/COPYING*


%package -n libvodman
Summary: vodman library.
Group: Development/Libraries
Provides: libvodman = %{version}

%description -n libvodman
%{summary}

%files -n libvodman
%defattr(-,root,root,-)
%{_libdir}/*


%package -n libvodman-devel
Summary: Development headers for vodman library.
Group: Development/Libraries
Requires: libvodman = %{version}
Requires: vodman-service = %{version}

%description -n libvodman-devel
%{summary}

%files -n libvodman-devel
%defattr(-,root,root,-)
%{_includedir}/vodman/*.h


%package -n vodman-service
Summary: vodman service.
Group: System Environment/Daemon
Requires: libvodman = %{version}
Requires: python3-base

%description -n vodman-service
%{summary}

%files -n vodman-service
%defattr(-,root,root,-)
%attr(0755,root,root) %{_bindir}/vodman-service
%attr(0755,root,root) %{_bindir}/vodman-cli
%attr(0755,root,root) %{_bindir}/vodman-youtube-dl
/usr/share/dbus-1/services/org.duckdns.jgressmann.vodman.service.service
/usr/share/dbus-1/interfaces/org.duckdns.jgressmann.vodman.service.xml



#%define __provides_exclude_from ^%{_datadir}/.*$
#%define __requires_exclude ^libvodman.*$

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5
%qtc_make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install


%define vs_pid $(ps -C vodman-service -o pid=)



%post -n vodman-service
if [ -n "%{vs_pid}" ]
then
    kill -s 10 %{vs_pid}
fi


%post -n libvodman
/sbin/ldconfig

%postun -n libvodman
/sbin/ldconfig
