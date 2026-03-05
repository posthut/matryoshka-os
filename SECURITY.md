# Security Policy

## Supported Versions

MatryoshkaOS is currently in active development. Security updates are provided for the following versions:

| Version | Supported          |
| ------- | ------------------ |
| main    | :white_check_mark: |
| develop | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

We take security seriously, even in an educational project. If you discover a security vulnerability, please follow these steps:

### 1. Do Not Open a Public Issue

Please **do not** create a public GitHub issue for security vulnerabilities.

### 2. Report Privately

Send an email to the project maintainers with:

- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

### 3. Response Timeline

- **Initial Response**: Within 48 hours
- **Status Update**: Within 7 days
- **Fix Timeline**: Depends on severity

## Security Considerations

### Current Status

MatryoshkaOS is an **educational operating system** and should **not** be used in production environments. Known security limitations include:

- No memory protection between processes (planned)
- No user authentication (planned)
- No encryption (planned)
- No secure boot chain validation (uses GRUB2)
- Limited input validation

### Planned Security Features

As the project develops, we plan to implement:

- [x] GRUB2 bootloader (industry-standard, audited)
- [ ] Memory protection (MMU, page permissions)
- [ ] Process isolation
- [ ] System call validation
- [ ] Buffer overflow protection
- [ ] Stack canaries
- [ ] ASLR (Address Space Layout Randomization)
- [ ] Secure Boot support

## Best Practices

When contributing code, please:

- ✅ Validate all user inputs
- ✅ Check buffer boundaries
- ✅ Handle error conditions properly
- ✅ Avoid undefined behavior
- ✅ Use safe string operations
- ✅ Document security-relevant code

## Known Vulnerabilities

Currently known limitations (not considered vulnerabilities in educational context):

1. **No stack protection** - Stack overflow possible
2. **No heap protection** - Heap corruption possible
3. **Global kernel access** - No privilege separation yet
4. **No input sanitization** - Limited validation

These are accepted trade-offs during early development and will be addressed in future stages.

## Security Audit

This project welcomes security audits and reviews. If you're interested in performing a security review, please open an issue to coordinate.

## References

- [OWASP Secure Coding Practices](https://owasp.org/www-project-secure-coding-practices-quick-reference-guide/)
- [CWE - Common Weakness Enumeration](https://cwe.mitre.org/)
- [CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard)

