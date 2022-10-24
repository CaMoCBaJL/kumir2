%define branch master
%define git_hash 0d1c303eab080d67492ad0f9b14776036b59b9ef

%define oname kumir2

Name: kumir2-robots
Version: 2.1.0
Release: alt1

Summary: Development version of Kumir - simple programming language and IDE for teaching programming
Summary(ru_RU.UTF-8): Разрабатываемая версия системы Кумир - простого учебного языка программирования и среды разработки

License: GPL
Group: Education
Url: https://github.com/mastersin/kumir2
Packager: Evgeny Sinelnikov <sin@altlinux.org>

BuildRequires(pre): rpm-build-ubt
BuildRequires: qt5-base-devel qt5-svg-devel qt5-x11extras-devel qt5-script-devel
BuildRequires: gcc-c++ cmake python3 boost-devel python-modules-json

Conflicts: kumir2

Source: %oname-%version.tar
Patch: %oname-%version-alt.patch

%description
Implementation of Kumir programming language, designed by academician
Ershov. It has very simple syntax, also known as "Russian algorithmical
language". Includes compiler, runtime, IDE and  modules "Robot", "Draw",
"Turtle" and some others.

This is a second generation of well-known Kumir system.

%description -l ru_RU.UTF-8
Кумир - это учебный язык программирования, описанный в учебнике
А.Г.Кушниренко, и среда разработки. Он имеет простой синтаксис,
известный также как "русский алгоритмический язык". В состав среды также
входят канонические исполнители Робот, Чертежник, Черепаха и другие,
что делает Кумир очень удобным для начального обучения программированию.

Пакет kumir2 содержит новую реализацию системы Кумир.

%package devel
Summary:   Header files and libraries for kumir2
Group:     Development/C++
Requires:  %name = %version-%release

%description
This is development files of well-known Kumir system.

%description devel -l ru_RU.UTF-8
Пакет %name-devel содержит файлы для отладки и разработки системы Кумир.

%prep
%setup -n %oname-%version
%patch -p1
sed -i "s/^Categories=.*$/Categories=Education;Qt;ComputerScience;/" *.desktop

%build
%cmake -DUSE_QT=5 -DKUMIR2_DISABLED_SUBDIRS="" -DKUMIR2_ALL_TOOLS=ON
%cmake_build

%install
# make install
%cmakeinstall_std

%files
%_bindir/*
%_libdir/%oname
%_datadir/%oname
%_datadir/mime/packages/*.xml
%_desktopdir/*
%_iconsdir/*/*/*/*

%files devel
%_includedir/*
%_libdir/cmake/*

%changelog
* %(LC_TIME=C date "+%%a %%b %%d %%Y") %{?package_signer:%package_signer}%{!?package_signer:%packager} %version-%release
- Build unstable %branch branch as %name
- Version: %{branch}-%{git_hash}-%(LC_TIME=C date "+%%Y%%m%%d")

* Fri Aug 31 2018 Evgeny Sinelnikov <sin@altlinux.org> 2.1.0rcX-alt1%ubt
- Build unstable master branch as kumir-devel with ubt macros
- Version: master-1103a3358c1dc1742052316648a9b5783a155d4e-20180826

* Mon Oct 03 2016 Yekaterina Aleksanenkova <lex@altlinux.org> 2.1.0-alt1
- Stable release.

* Sun Jun 15 2014 Denis Kirienko <dk@altlinux.org> 2.1.0-alt0.beta5
- Version 2.1.0-beta5

* Thu Jan 09 2014 Denis Kirienko <dk@altlinux.org> 2.1.0-alt0.beta4
- Version 2.1.0-beta4

* Tue Nov 05 2013 Denis Kirienko <dk@altlinux.org> 2.0.99-alt1
- Version 2.1.0-beta1

* Tue Mar 20 2012 Sergey Bolshakov <sbolshakov@altlinux.ru> 1.99.0.alpha7-alt2
- rebuilt with optflags

* Sat Aug 20 2011 Denis Kirienko <dk@altlinux.ru> 1.99.0.alpha7-alt1
- Initial build for Sisyphus - alpha-version, only for testing.
