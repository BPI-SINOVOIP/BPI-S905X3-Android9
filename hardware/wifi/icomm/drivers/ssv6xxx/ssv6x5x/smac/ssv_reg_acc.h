/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SSV_REG_ACC_H_
#define _SSV_REG_ACC_H_ 
#define SMAC_REG_WRITE(_s,_r,_v) \
        ({ \
            typeof(_s) __s = _s; \
            if ( _s->sc->log_ctrl & LOG_REGW) \
                printk("w a:0x%x d:0x%x\n", _r ,_v); \
            __s->hci.hci_ops->hci_write_word(__s->hci.hci_ctrl, _r,_v); \
        })
#define SMAC_REG_READ(_s,_r,_v) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_read_word(__s->hci.hci_ctrl, _r, _v); \
        })
#define SMAC_REG_SAFE_WRITE(_s,_r,_v) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_safe_write_word(__s->hci.hci_ctrl, _r,_v); \
        })
#define SMAC_REG_SAFE_READ(_s,_r,_v) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_safe_read_word(__s->hci.hci_ctrl, _r, _v); \
        })
#ifndef __x86_64
#define SMAC_RF_REG_READ(_s,_r,_v) SMAC_REG_READ(_s, _r, _v)
#else
#define SMAC_RF_REG_READ(_s,_r,_v) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_read_word(__s->hci.hci_ctrl, _r, _v); \
            __s->hci.hci_ops->hci_read_word(__s->hci.hci_ctrl, _r, _v); \
        })
#endif
#define SMAC_BURST_REG_SAFE_READ(_s,_r,_v,_n) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_burst_safe_read_word(__s->hci.hci_ctrl, _r, _v, _n); \
        })
#define SMAC_BURST_REG_SAFE_WRITE(_s,_r,_v,_n) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_burst_safe_write_word(__s->hci.hci_ctrl, _r, _v, _n); \
        })
#define SMAC_BURST_REG_READ(_s,_r,_v,_n) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_burst_read_word(__s->hci.hci_ctrl, _r, _v, _n); \
        })
#define SMAC_BURST_REG_WRITE(_s,_r,_v,_n) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_burst_write_word(__s->hci.hci_ctrl, _r, _v, _n); \
        })
#define SMAC_LOAD_FW(_s,_r,_v) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_load_fw(__s->hci.hci_ctrl, _r, _v); \
        })
#define SMAC_IFC_RESET(_s) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_interface_reset(__s->hci.hci_ctrl); \
        })
#define SMAC_SYSPLF_RESET(_s,_r,_v) \
        ({ \
            typeof(_s) __s = _s; \
            __s->hci.hci_ops->hci_sysplf_reset(__s->hci.hci_ctrl, _r, _v); \
        })
#define SMAC_REG_CONFIRM(_s,_r,_v) \
{ \
    u32 _regval; \
    SMAC_REG_READ(_s, _r, &_regval); \
    if (_regval != (_v)) { \
        printk("ERROR!!Please check interface!\n"); \
        printk("[0x%08x]: 0x%08x!=0x%08x\n", \
        (_r), (_v), _regval); \
        printk("SOS!SOS!\n"); \
        return -1; \
    } \
}
#define SMAC_REG_SET_BITS(_sh,_reg,_set,_clr) \
({ \
    int ret; \
    u32 _regval; \
    ret = SMAC_REG_READ(_sh, _reg, &_regval); \
    _regval &= ~(_clr); \
    _regval |= (_set); \
    if (ret == 0) \
        ret = SMAC_REG_WRITE(_sh, _reg, _regval); \
    ret; \
})
#define SMAC_REG_SAFE_SET_BITS(_sh,_reg,_set,_clr) \
({ \
    int ret; \
    u32 _regval; \
    ret = SMAC_REG_SAFE_READ(_sh, _reg, &_regval); \
    _regval &= ~(_clr); \
    _regval |= (_set); \
    if (ret == 0) \
        ret = SMAC_REG_SAFE_WRITE(_sh, _reg, _regval); \
    ret; \
})
#ifndef __x86_64
#define SMAC_RF_REG_SET_BITS(_sh,_reg,_set,_clr) SMAC_REG_SET_BITS(_sh, _reg, _set, _clr)
#else
#define SMAC_RF_REG_SET_BITS(_sh,_reg,_set,_clr) \
({ \
    int ret; \
    u32 _regval; \
    ret = SMAC_REG_READ(_sh, _reg, &_regval); \
    ret = SMAC_REG_READ(_sh, _reg, &_regval); \
    _regval &= ~(_clr); \
    _regval |= (_set); \
    if (ret == 0) \
        ret = SMAC_REG_WRITE(_sh, _reg, _regval); \
    ret; \
})
#endif
#endif
