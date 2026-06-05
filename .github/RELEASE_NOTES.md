## Template vX.Y.Z

<!--
  Notes for the release currently being tagged.
  This file is used as the body of the GitHub Release (softprops/action-gh-release
  appends the auto-generated changelog below it). Replace this content before
  pushing a new `vX.Y.Z` tag.
-->

### What's new

- ...

### Requirements

- Windows 10/11 (test signing must be enabled)
- Visual C++ Redistributable 2022

### Installation

**1. Enable test signing** (run as Administrator, then reboot):
```bat
bcdedit /set testsigning on
```

**2. Install the certificate** (run as Administrator):
```bat
certutil -addstore Root Template.cer
certutil -addstore TrustedPublisher Template.cer
```

**3. Install the driver**:
```bat
sc create Template type= kernel binPath= C:\path\to\Template.sys
sc start Template
```

### Packages

| Package | Framework | Config |
|---------|-----------|--------|
| `Template-kmdf-rel.zip` | KMDF | Release |
| `Template-wdm-rel.zip`  | WDM  | Release |
| `Template-kmdf-dbg.zip` | KMDF | Debug   |
| `Template-wdm-dbg.zip`  | WDM  | Debug   |
