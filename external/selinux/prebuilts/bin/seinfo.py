#!/usr/bin/env python
# Copyright 2014-2015, Tresys Technology, LLC
#
# This file is part of SETools.
#
# SETools is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# SETools is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with SETools.  If not, see <http://www.gnu.org/licenses/>.
#

from __future__ import print_function
import setools
import argparse
import sys
import logging


def expand_attr(attr):
    """Render type and role attributes."""
    items = "\n\t".join(sorted(str(i) for i in attr.expand()))
    contents = items if items else "<empty attribute>"
    return "{0}\n\t{1}".format(attr.statement(), contents)

parser = argparse.ArgumentParser(
    description="SELinux policy information tool.")
parser.add_argument("--version", action="version", version=setools.__version__)
parser.add_argument("policy", help="Path to the SELinux policy to query.", nargs="?")
parser.add_argument("-x", "--expand", action="store_true",
                    help="Print additional information about the specified components.")
parser.add_argument("--flat",  help="Print without item count nor indentation.",
                    dest="flat", default=False, action="store_true")
parser.add_argument("-v", "--verbose", action="store_true",
                    help="Print extra informational messages")
parser.add_argument("--debug", action="store_true", dest="debug", help="Enable debugging.")

queries = parser.add_argument_group("Component Queries")
queries.add_argument("-a", "--attribute",  help="Print type attributes.", dest="typeattrquery",
                     nargs='?', const=True, metavar="ATTR")
queries.add_argument("-b", "--bool", help="Print Booleans.", dest="boolquery",
                     nargs='?', const=True, metavar="BOOL")
queries.add_argument("-c", "--class", help="Print object classes.", dest="classquery",
                     nargs='?', const=True, metavar="CLASS")
queries.add_argument("-r", "--role", help="Print roles.", dest="rolequery",
                     nargs='?', const=True, metavar="ROLE")
queries.add_argument("-t", "--type", help="Print types.", dest="typequery",
                     nargs='?', const=True, metavar="TYPE")
queries.add_argument("-u", "--user", help="Print users.", dest="userquery",
                     nargs='?', const=True, metavar="USER")
queries.add_argument("--category", help="Print MLS categories.", dest="mlscatsquery",
                     nargs='?', const=True, metavar="CAT")
queries.add_argument("--common", help="Print common permission set.", dest="commonquery",
                     nargs='?', const=True, metavar="COMMON")
queries.add_argument("--constrain", help="Print constraints.", dest="constraintquery",
                     nargs='?', const=True, metavar="CLASS")
queries.add_argument("--default", help="Print default_* rules.", dest="defaultquery",
                     nargs='?', const=True, metavar="CLASS")
queries.add_argument("--fs_use", help="Print fs_use statements.", dest="fsusequery",
                     nargs='?', const=True, metavar="FS_TYPE")
queries.add_argument("--genfscon", help="Print genfscon statements.", dest="genfsconquery",
                     nargs='?', const=True, metavar="FS_TYPE")
queries.add_argument("--initialsid", help="Print initial SIDs (contexts).", dest="initialsidquery",
                     nargs='?', const=True, metavar="NAME")
queries.add_argument("--netifcon", help="Print netifcon statements.", dest="netifconquery",
                     nargs='?', const=True, metavar="DEVICE")
queries.add_argument("--nodecon", help="Print nodecon statements.", dest="nodeconquery",
                     nargs='?', const=True, metavar="ADDR")
queries.add_argument("--permissive", help="Print permissive types.", dest="permissivequery",
                     nargs='?', const=True, metavar="TYPE")
queries.add_argument("--polcap", help="Print policy capabilities.", dest="polcapquery",
                     nargs='?', const=True, metavar="NAME")
queries.add_argument("--portcon", help="Print portcon statements.", dest="portconquery",
                     nargs='?', const=True, metavar="PORTNUM[-PORTNUM]")
queries.add_argument("--sensitivity", help="Print MLS sensitivities.", dest="mlssensquery",
                     nargs='?', const=True, metavar="SENS")
queries.add_argument("--typebounds", help="Print typebounds statements.", dest="typeboundsquery",
                     nargs='?', const=True, metavar="BOUND_TYPE")
queries.add_argument("--validatetrans", help="Print validatetrans.", dest="validatetransquery",
                     nargs='?', const=True, metavar="CLASS")
queries.add_argument("--all", help="Print all of the above.  On a Xen policy, the Xen components "
                     "will also be printed", dest="all", default=False, action="store_true")

xen = parser.add_argument_group("Xen Component Queries")
xen.add_argument("--ioportcon", help="Print all ioportcon statements.", dest="ioportconquery",
                 default=False, action="store_true")
xen.add_argument("--iomemcon", help="Print all iomemcon statements.", dest="iomemconquery",
                 default=False, action="store_true")
xen.add_argument("--pcidevicecon", help="Print all pcidevicecon statements.",
                 dest="pcideviceconquery", default=False, action="store_true")
xen.add_argument("--pirqcon", help="Print all pirqcon statements.", dest="pirqconquery",
                 default=False, action="store_true")
xen.add_argument("--devicetreecon", help="Print all devicetreecon statements.",
                 dest="devicetreeconquery", default=False, action="store_true")


args = parser.parse_args()

if args.debug:
    logging.basicConfig(level=logging.DEBUG,
                        format='%(asctime)s|%(levelname)s|%(name)s|%(message)s')
elif args.verbose:
    logging.basicConfig(level=logging.INFO, format='%(message)s')
else:
    logging.basicConfig(level=logging.WARNING, format='%(message)s')

try:
    p = setools.SELinuxPolicy(args.policy)
    components = []

    if args.boolquery or args.all:
        q = setools.BoolQuery(p)
        if isinstance(args.boolquery, str):
            q.name = args.boolquery

        components.append(("Booleans", q, lambda x: x.statement()))

    if args.mlscatsquery or args.all:
        q = setools.CategoryQuery(p)
        if isinstance(args.mlscatsquery, str):
            q.name = args.mlscatsquery

        components.append(("Categories", q, lambda x: x.statement()))

    if args.classquery or args.all:
        q = setools.ObjClassQuery(p)
        if isinstance(args.classquery, str):
            q.name = args.classquery

        components.append(("Classes", q, lambda x: x.statement()))

    if args.commonquery or args.all:
        q = setools.CommonQuery(p)
        if isinstance(args.commonquery, str):
            q.name = args.commonquery

        components.append(("Commons", q, lambda x: x.statement()))

    if args.constraintquery or args.all:
        q = setools.ConstraintQuery(p, ruletype=["constrain", "mlsconstrain"])
        if isinstance(args.constraintquery, str):
            q.tclass = [args.constraintquery]

        components.append(("Constraints", q, lambda x: x.statement()))

    if args.defaultquery or args.all:
        q = setools.DefaultQuery(p)
        if isinstance(args.defaultquery, str):
            q.tclass = [args.defaultquery]

        components.append(("Default rules", q, lambda x: x.statement()))

    if args.fsusequery or args.all:
        q = setools.FSUseQuery(p)
        if isinstance(args.fsusequery, str):
            q.fs = args.fsusequery

        components.append(("Fs_use", q, lambda x: x.statement()))

    if args.genfsconquery or args.all:
        q = setools.GenfsconQuery(p)
        if isinstance(args.genfsconquery, str):
            q.fs = args.genfsconquery

        components.append(("Genfscon", q, lambda x: x.statement()))

    if args.initialsidquery or args.all:
        q = setools.InitialSIDQuery(p)
        if isinstance(args.initialsidquery, str):
            q.name = args.initialsidquery

        components.append(("Initial SIDs", q, lambda x: x.statement()))

    if args.netifconquery or args.all:
        q = setools.NetifconQuery(p)
        if isinstance(args.netifconquery, str):
            q.name = args.netifconquery

        components.append(("Netifcon", q, lambda x: x.statement()))

    if args.nodeconquery or args.all:
        q = setools.NodeconQuery(p)
        if isinstance(args.nodeconquery, str):
            q.network = args.nodeconquery

        components.append(("Nodecon", q, lambda x: x.statement()))

    if args.permissivequery or args.all:
        q = setools.TypeQuery(p, permissive=True, match_permissive=True)
        if isinstance(args.permissivequery, str):
            q.name = args.permissivequery

        components.append(("Permissive Types", q, lambda x: x.statement()))

    if args.polcapquery or args.all:
        q = setools.PolCapQuery(p)
        if isinstance(args.polcapquery, str):
            q.name = args.polcapquery

        components.append(("Polcap", q, lambda x: x.statement()))

    if args.portconquery or args.all:
        q = setools.PortconQuery(p)
        if isinstance(args.portconquery, str):
            try:
                ports = [int(i) for i in args.portconquery.split("-")]
            except ValueError:
                parser.error("Enter a port number or range, e.g. 22 or 6000-6020")

            if len(ports) == 2:
                q.ports = ports
            elif len(ports) == 1:
                q.ports = (ports[0], ports[0])
            else:
                parser.error("Enter a port number or range, e.g. 22 or 6000-6020")

        components.append(("Portcon", q, lambda x: x.statement()))

    if args.rolequery or args.all:
        q = setools.RoleQuery(p)
        if isinstance(args.rolequery, str):
            q.name = args.rolequery

        components.append(("Roles", q, lambda x: x.statement()))

    if args.mlssensquery or args.all:
        q = setools.SensitivityQuery(p)
        if isinstance(args.mlssensquery, str):
            q.name = args.mlssensquery

        components.append(("Sensitivities", q, lambda x: x.statement()))

    if args.typeboundsquery or args.all:
        q = setools.BoundsQuery(p, ruletype=["typebounds"])
        if isinstance(args.typeboundsquery, str):
            q.child = args.typeboundsquery

        components.append(("Typebounds", q, lambda x: x.statement()))

    if args.typequery or args.all:
        q = setools.TypeQuery(p)
        if isinstance(args.typequery, str):
            q.name = args.typequery

        components.append(("Types", q, lambda x: x.statement()))

    if args.typeattrquery or args.all:
        q = setools.TypeAttributeQuery(p)
        if isinstance(args.typeattrquery, str):
            q.name = args.typeattrquery

        components.append(("Type Attributes", q, expand_attr))

    if args.userquery or args.all:
        q = setools.UserQuery(p)
        if isinstance(args.userquery, str):
            q.name = args.userquery

        components.append(("Users", q, lambda x: x.statement()))

    if args.validatetransquery or args.all:
        q = setools.ConstraintQuery(p, ruletype=["validatetrans", "mlsvalidatetrans"])
        if isinstance(args.validatetransquery, str):
            q.tclass = [args.validatetransquery]

        components.append(("Validatetrans", q, lambda x: x.statement()))

    if p.target_platform == "xen":
        if args.ioportconquery or args.all:
            q = setools.IoportconQuery(p)
            components.append(("Ioportcon", q, lambda x: x.statement()))

        if args.iomemconquery or args.all:
            q = setools.IomemconQuery(p)
            components.append(("Iomemcon", q, lambda x: x.statement()))

        if args.pcideviceconquery or args.all:
            q = setools.PcideviceconQuery(p)
            components.append(("Pcidevicecon", q, lambda x: x.statement()))

        if args.pirqconquery or args.all:
            q = setools.PirqconQuery(p)
            components.append(("Pirqcon", q, lambda x: x.statement()))

        if args.devicetreeconquery or args.all:
            q = setools.DevicetreeconQuery(p)
            components.append(("Devicetreecon", q, lambda x: x.statement()))

    if (not components or args.all) and not args.flat:
        mls = "enabled" if p.mls else "disabled"

        print("Statistics for policy file: {0}".format(p))
        print("Policy Version:             {0} (MLS {1})".format(p.version, mls))
        print("Target Policy:              {0}".format(p.target_platform))
        print("Handle unknown classes:     {0}".format(p.handle_unknown))
        print("  Classes:         {0:7}    Permissions:     {1:7}".format(
            p.class_count, p.permission_count))
        print("  Sensitivities:   {0:7}    Categories:      {1:7}".format(
            p.level_count, p.category_count))
        print("  Types:           {0:7}    Attributes:      {1:7}".format(
            p.type_count, p.type_attribute_count))
        print("  Users:           {0:7}    Roles:           {1:7}".format(
            p.user_count, p.role_count))
        print("  Booleans:        {0:7}    Cond. Expr.:     {1:7}".format(
            p.boolean_count, p.conditional_count))
        print("  Allow:           {0:7}    Neverallow:      {1:7}".format(
            p.allow_count, p.neverallow_count))
        print("  Auditallow:      {0:7}    Dontaudit:       {1:7}".format(
            p.auditallow_count, p.dontaudit_count))
        print("  Type_trans:      {0:7}    Type_change:     {1:7}".format(
            p.type_transition_count, p.type_change_count))
        print("  Type_member:     {0:7}    Range_trans:     {1:7}".format(
            p.type_member_count, p.range_transition_count))
        print("  Role allow:      {0:7}    Role_trans:      {1:7}".format(
            p.role_allow_count, p.role_transition_count))
        print("  Constraints:     {0:7}    Validatetrans:   {1:7}".format(
            p.constraint_count, p.validatetrans_count))
        print("  MLS Constrain:   {0:7}    MLS Val. Tran:   {1:7}".format(
            p.mlsconstraint_count, p.mlsvalidatetrans_count))
        print("  Permissives:     {0:7}    Polcap:          {1:7}".format(
            p.permissives_count, p.polcap_count))
        print("  Defaults:        {0:7}    Typebounds:      {1:7}".format(
            p.default_count, p.typebounds_count))

        if p.target_platform == "selinux":
            print("  Allowxperm:      {0:7}    Neverallowxperm: {1:7}".format(
                p.allowxperm_count, p.neverallowxperm_count))
            print("  Auditallowxperm: {0:7}    Dontauditxperm:  {1:7}".format(
                p.auditallowxperm_count, p.dontauditxperm_count))
            print("  Initial SIDs:    {0:7}    Fs_use:          {1:7}".format(
                p.initialsids_count, p.fs_use_count))
            print("  Genfscon:        {0:7}    Portcon:         {1:7}".format(
                p.genfscon_count, p.portcon_count))
            print("  Netifcon:        {0:7}    Nodecon:         {1:7}".format(
                p.netifcon_count, p.nodecon_count))
        elif p.target_platform == "xen":
            print("  Initial SIDs:    {0:7}    Devicetreecon    {1:7}".format(
                p.initialsids_count, p.devicetreecon_count))
            print("  Iomemcon:        {0:7}    Ioportcon:       {1:7}".format(
                p.iomemcon_count, p.ioportcon_count))
            print("  Pcidevicecon:    {0:7}    Pirqcon:         {1:7}".format(
                p.pcidevicecon_count, p.pirqcon_count))

    for desc, component, expander in components:
        results = sorted(component.results())
        if not args.flat:
            print("\n{0}: {1}".format(desc, len(results)))
        for item in results:
            result = expander(item) if args.expand else item
            strfmt = "   {0}" if not args.flat else "{0}"
            print(strfmt.format(result))

except Exception as err:
    if args.debug:
        logging.exception(str(err))
    else:
        print(err)

    sys.exit(1)
