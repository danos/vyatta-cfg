interfaces {
    dataplane dp0p2p1 {
        address dhcp
    }
    dataplane dp0port1 {
        address 2001:470:1f04:1c62::1/64
        disable
    }
    loopback lo {
    }
}
protocols {
    static {
        route 10.0.0.0/8 {
            next-hop 192.168.122.1 {
            }
        }
        route6 2001:470:1f04::/64 {
            next-hop 2001:470:1f04:1c62::2 {
            }
        }
    }
}
qos-policy Test {
    default benchmark
    map {
        dscp 4 {
            to 1
        }
    }
    profile benchmark {
        bandwidth unlimited
    }
    queue 1 {
        bandwidth 10Mbps
    }
}
service {
    snmp {
        community public {
            client ::
            client 2001:470:1f04:1c62::2
        }
    }
    ssh {
        disable-host-validation
    }
}
system {
    acm {
        enable
        operational-ruleset {
            rule 9988 {
                action deny
                command /show/configuration
                group vyattaop
            }
            rule 9989 {
                action allow
                command "/clear/*"
                group vyattaop
            }
            rule 9990 {
                action allow
                command "/show/*"
                group vyattaop
            }
            rule 9991 {
                action allow
                command "/monitor/*"
                group vyattaop
            }
            rule 9992 {
                action allow
                command "/ping/*"
                group vyattaop
            }
            rule 9993 {
                action allow
                command "/reset/*"
                group vyattaop
            }
            rule 9994 {
                action allow
                command "/release/*"
                group vyattaop
            }
            rule 9995 {
                action allow
                command "/renew/*"
                group vyattaop
            }
            rule 9996 {
                action allow
                command "/telnet/*"
                group vyattaop
            }
            rule 9997 {
                action allow
                command "/traceroute/*"
                group vyattaop
            }
            rule 9998 {
                action allow
                command "/update/*"
                group vyatta-op
            }
            rule 9999 {
                action deny
                command "*"
                group vyattaop
            }
        }
        ruleset {
            rule 9999 {
                action allow
                group vyattacfg
                operation "*"
                path "*"
            }
        }
    }
    config-management {
        commit-revisions 20
    }
    console {
        device ttyS0 {
            speed 9600
        }
    }
    login {
        user super {
            authentication {
                encrypted-password $1$ntb0Xy5Y$FygFCclBIGCgLR5IfAzMx1
            }
            level superuser
        }
        user vyatta {
            authentication {
                encrypted-password $1$gBA8RPAZ$MRK6jrQiD/X0kpPZ4dmlP0
            }
            level admin
        }
    }
    ntp {
        server 0.vyatta.pool.ntp.org {
        }
        server 1.vyatta.pool.ntp.org {
        }
        server 2.vyatta.pool.ntp.org {
        }
    }
    options {
        reboot-on-panic false
    }
    syslog {
        global {
            facility all {
                level notice
            }
            facility dataplane {
                level debug
            }
            facility protocols {
                level debug
            }
        }
        host 192.168.122.1 {
            facility all {
                level err
            }
        }
    }
    time-zone Canada/Pacific
}


/* Warning: Do not remove the following line. */
/* === vyatta-config-version: "config-management@1:dhcp-relay@1:dhcp-server@4:ipsec@4:pim@1:quagga@3:system@10:vrrp@1:webgui@1" === */
/* Release version: 999.master.01300023 */
