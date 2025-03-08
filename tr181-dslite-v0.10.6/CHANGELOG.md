# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]


## Release v0.10.6 - 2024-09-25(12:53:54 +0000)

### Other

- - [USP][CDRouter][Random] Some datamodel path are missing in USP hl-api tests
- - [USP][CDRouter][Random] Some datamodel path are missing in USP hl-api tests

## Release v0.10.5 - 2024-09-10(07:16:15 +0000)

### Other

- [AppArmor] Create AppAmor profile for plugins

## Release v0.10.4 - 2024-07-23(07:50:25 +0000)

### Fixes

- Better shutdown script

## Release v0.10.3 - 2024-07-08(06:58:23 +0000)

## Release v0.10.2 - 2024-06-27(16:49:23 +0000)

### Other

- amx plugin should not run as root user

## Release v0.10.1 - 2024-04-10(07:07:44 +0000)

### Changes

- Make amxb timeouts configurable

## Release v0.10.0 - 2024-02-05(16:22:02 +0000)

### New

- Add tr181-device proxy odl files to components

## Release v0.9.0 - 2024-01-17(09:33:35 +0000)

### New

- Add tr181-device proxy odl files to components

## Release v0.8.2 - 2023-10-23(09:43:44 +0000)

### Fixes

- NetDev MIB takes very long to synchronize when dslite0 interface comes up

## Release v0.8.1 - 2023-10-13(14:13:16 +0000)

### Changes

-  All applications using sahtrace logs should use default log levels

## Release v0.8.0 - 2023-10-12(11:27:22 +0000)

### New

- [DSLITE][MSS] Adapt MSS for the dslite traffic depending on the MTU of the underlying interface.

## Release v0.7.0 - 2023-10-11(12:37:05 +0000)

### New

- [amxrt][no-root-user][capability drop] All amx plugins must be adapted to run as non-root and lmited capabilities

## Release v0.6.6 - 2023-09-22(16:20:41 +0000)

### Fixes

- [OJO][Nokia]Website navigation does not behave normally for LAN and wlan Devices

## Release v0.6.5 - 2023-07-03(12:43:12 +0000)

### Fixes

- Init script has no shutdown function

## Release v0.6.4 - 2023-06-15(10:28:08 +0000)

### Fixes

- Missing Device prefix in Tunnel(ed)Interface parameters

## Release v0.6.3 - 2023-05-18(05:36:25 +0000)

### Fixes

- [prplOS-next][tr181-dslite] tr181-dslite crashes

## Release v0.6.2 - 2023-05-11(09:18:03 +0000)

### Other

- [Coverage] Remove SAHTRACE defines in order to increase branching coverage

## Release v0.6.1 - 2023-04-27(06:21:13 +0000)

### Fixes

- tr181-dslite.sh: fix not starting service

## Release v0.6.0 - 2023-03-23(15:23:09 +0000)

### Other

- Use sah_libc-ares instead of opensource_c-ares

## Release v0.5.9 - 2023-03-09(09:16:12 +0000)

### Other

- [Config] enable configurable coredump generation

## Release v0.5.8 - 2023-03-06(09:54:09 +0000)

### Fixes

- Fix c-ares usage in DSLite

## Release v0.5.7 - 2023-02-18(07:27:23 +0000)

### Fixes

- [dslite] EndpointName is not always persistent and writable when EndpointAssignmentPrecedence is Static

## Release v0.5.6 - 2023-02-02(15:10:11 +0000)

### Fixes

- EndpointName not always modifiable when EndpointAssignmentPrecedence is Static

## Release v0.5.5 - 2023-01-26(07:55:54 +0000)

### Fixes

- Fix origin field in DSLite data model

## Release v0.5.4 - 2023-01-24(09:53:30 +0000)

### Fixes

- Remove dslite0 interface when DSLite.InterfaceSetting.1.Status == Error

## Release v0.5.3 - 2023-01-20(13:44:11 +0000)

### Fixes

- Bug: dslite doesn't always come up after a reboot

## Release v0.5.2 - 2023-01-10(11:45:24 +0000)

### Fixes

- avoidable copies of strings, htables and lists

## Release v0.5.1 - 2023-01-09(09:36:34 +0000)

### Fixes

- event handler filter uses regexp on string literal

## Release v0.5.0 - 2022-12-19(16:25:43 +0000)

### New

- Go from 2 to 1 default InterfaceSetting instances in DSLite

## Release v0.4.6 - 2022-12-13(13:27:16 +0000)

### Fixes

- Correct linking with /opt/prplos

## Release v0.4.5 - 2022-12-11(14:47:44 +0000)

### Other

- Add runtime dependencies "kmod-ip6-tunnel" and "kmod-iptunnel6"

## Release v0.4.4 - 2022-12-09(09:40:36 +0000)

### Fixes

- [Config] coredump generation should be configurable

## Release v0.4.3 - 2022-12-06(16:35:05 +0000)

### Changes

- Fix issue with Enable parameters

## Release v0.4.2 - 2022-11-30(15:39:42 +0000)

### Other

- Add config option to link with c-ares under /opt/prplos

## Release v0.4.1 - 2022-11-24(14:15:43 +0000)

### Other

- Cleanup administrative issues

## Release v0.4.0 - 2022-11-23(09:38:17 +0000)

### New

- Configure dslite0 network interface

## Release v0.3.2 - 2022-11-22(12:58:34 +0000)

### Other

- Enable auto opensource

## Release v0.3.1 - 2022-11-22(12:50:34 +0000)

### Other

- Opensource component

## Release v0.3.0 - 2022-11-10(12:26:22 +0000)

### New

- DSLite async DNS resolving

## Release v0.2.0 - 2022-10-20(15:00:06 +0000)

### New

- DSLite Initial working plugin

## Release v0.1.1 - 2022-10-11(09:30:34 +0000)

### Fixes

- Implement TR181 compatible DSLite plugin

## Release v0.1.0 - 2022-09-22(07:20:35 +0000)

### New

- Implement TR181 compatible DSLite plugin.

### Fixes

- Remove pedantic flag

