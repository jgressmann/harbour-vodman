%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

Name:       harbour-vodman
Summary:    Video On Demand (VOD) manager for SailfishOS
Version:    1.0.0
Release:    1
#Group:      Applications/Multimedia
Group:      Qt/Qt
License:    MIT
#URL:        http://foo.bar
Source0:    %{name}-%{version}.tar.bz2

#BuildRequires:  pkgconfig(nemotransferengine-qt5)
BuildRequires:  pkgconfig(Qt5DBus)
#BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
#BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(sailfishapp) >= 0.0.10
BuildRequires:  desktop-file-utils
BuildRequires:  zip
BuildRequires:  python3-base

#Requires:   libnemotransferengine-qt5
#Requires:   nemo-transferengine-qt5
Requires:   python3-base
Requires:   sailfishsilica-qt5 >= 0.10.9
#Requires:   qt5-qtdeclarative-import-settings # for settings
#Requires:   qt5-qtdeclarative-import-folderlistmodel # folder picker


%description
%{summary}


%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5
%qtc_make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install


%define vs_pid $(pgrep -f vodman-service)


%post
/usr/bin/update-desktop-database -q
if [ -n "%{vs_pid}" ]
then
    kill -s 10 %{vs_pid}
fi

%files
%defattr(-,root,root,-)
#%attr(0755,root,root) /usr/lib/nemo-transferengine/plugins/libyoutube-dl.so
%attr(0755,root,root) %{_bindir}/%{name}
%attr(0755,root,root) %{_datadir}/%{name}/bin/youtube-dl
%attr(0755,root,root) %{_datadir}/%{name}/bin/vodman-cli
%attr(0755,root,root) %{_datadir}/%{name}/bin/vodman-service
%attr(0755,root,root) %{_libdir}/libvodman*
#%defattr(-,root,root,-)
%{_datadir}/applications/%{name}.desktop
#%{_datadir}/applications/open-url.desktop
#%{_datadir}/%{name}/*
#%{_datadir}/translations/sailfish-browser_eng_en.qm
#%{_datadir}/dbus-1/services/*.service
%{_datadir}/icons/hicolor/*/apps/%{name}.png
#%{_oneshotdir}/*
#%defattr(-,root,root,-)
/usr/share/dbus-1/services/org.duckdns.jgressmann.vodman.service.service
/usr/include/vodman/*
%{_datadir}/%{name}/qml/*
%{_datadir}/%{name}/icons/*
