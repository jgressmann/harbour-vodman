%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

Name:       harbour-vodman
Summary:    Video On Demand (VOD) download tool for SailfishOS
Version:    1.1.0
Release:    1
Group:      Applications/Multimedia
License:    MIT
URL:        https://openrepos.net/content/jgressmann/vodman
Source0:    %{name}-%{version}.tar.bz2

BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(sailfishapp) >= 0.0.10
BuildRequires:  desktop-file-utils


#Requires:   python3-base
Requires:   sailfishsilica-qt5 >= 0.10.9

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

