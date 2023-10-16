import io
from collections import defaultdict, namedtuple
import pyomo.environ as pyomo
import pandas
from timeit import default_timer as timer
import argparse
import os

parser = argparse.ArgumentParser()
parser.add_argument("nGat")
parser.add_argument("nPla")
parser.add_argument("nDev")
parser.add_argument("seed")
parser.add_argument("qos_bound")

args = parser.parse_args()

os.system("date")

DEBUG = False
VERBOSE = False
PACKET_SIZE = 400  # bits
QOS_LOWER_BOUND = float(args.qos_bound)
LOSS_HIGHER_BOUND = 4
TRANSMISSION_PROBABILITY = 0.01
MAX_RKC = 6835.94
MIN_RKC = 183.11
MAX_DELAY = PACKET_SIZE / MIN_RKC

startPre = timer()

cwd = os.getcwd()
path_output = cwd + "/data/model/output/"
path_data = cwd + "/data/model/"
gateway_data = cwd + "/data/placement/"
devices_data = cwd + "/data/placement/"

end_device_position_file = devices_data + "endDevices_LNM_Placement_" + str(args.seed) + "s+" + str(args.nDev) + "d.dat"
#"endDevices_LNM_Placement_1s+10d.dat"
if args.nPla == '1':
    gateway_position_file = gateway_data + "equidistantPlacement_" + str(args.nGat) + ".dat"
else:
    gateway_position_file = gateway_data + "equidistantPlacement_" + str(args.nGat) + "x" + str(args.nPla) + ".dat"

slice_association_file = path_data + "skl_" + \
                         str(args.seed) + "s_" + \
                         str(args.nGat) + "x" + \
                         str(args.nPla) +"Gv_" + \
                         str(args.nDev) + "D.dat"

plr1_file = path_data + "plrI_" + \
                         str(args.seed) + "s_" + \
                         str(args.nGat) + "x" + \
                         str(args.nPla) +"Gv_" + \
                         str(args.nDev) + "D.dat"

#DEBUG filenames
if DEBUG:
    print(end_device_position_file)
    print(gateway_position_file)
    print(slice_association_file)
    print(plr1_file)

#exit()

Position = namedtuple('Position', ['x', 'y', 'z'])
Configuration = namedtuple('Configuration', ['sf', 'tp'])
Gateway = namedtuple('Gateway', ['bandwidths', 'max_datarates'])

# -------------------------------
# Processing End Device Positions
# -------------------------------

end_device_position_df = pandas.read_csv(end_device_position_file, names=['x', 'y', 'z'],
                                         sep=" ", index_col=False)

end_device_positions = {}
for row in end_device_position_df.itertuples():
    end_device_positions[row.Index] = Position(x=row.x, y=row.y, z=row.z)

if VERBOSE:
    print('End Device Positions: ')
    print(end_device_positions)
    print('------------------------------------------------------------------------------')

# ---------------------------------------
# Processing End Device Slice Association
# ---------------------------------------

slice_association_df = pandas.read_csv(slice_association_file, names=['device', 'slice'],
                                       sep=" ", index_col=False)

slice_associations = {}
for row in slice_association_df.itertuples():
    slice_associations[row.device] = row.slice

if VERBOSE:
    print('Slice Associations: ')
    print(slice_associations)
    print('------------------------------------------------------------------------------')

# -------------------------------
#       Processing Slices
# -------------------------------

slices = {0: 'slice1', 1: 'slice2', 2: 'slice3'}

if VERBOSE:
    print('Slices: ')
    print(slices)
    print('------------------------------------------------------------------------------')

# -----------------------------
# Processing Gateway Positions
# -----------------------------

gateway_position_df = pandas.read_csv(gateway_position_file, names=['x', 'y', 'z'],
                                      sep=" ", index_col=False)

gateway_positions = {}
for row in gateway_position_df.itertuples():
    gateway_positions[row.Index + len(end_device_positions)] = Position(x=row.x, y=row.y, z=row.z)

if VERBOSE:
    print('Gateway Positions: ')
    print(gateway_positions)
    print('------------------------------------------------------------------------------')

# -----------------------------
#      Processing Gateways
# -----------------------------

gateways = {0: Gateway(bandwidths=[125000.0 for i in enumerate(slices)],
                       max_datarates=[15197.75390625 for i in enumerate(slices)])}

if VERBOSE:
    print('Gateways: ')
    print(gateways)
    print('------------------------------------------------------------------------------')

# -----------------------------
#   Processing Configurations
# -----------------------------

spreading_factors = range(7, 13)
transmission_powers = range(2, 16, 2)
configurations = {}
for idx, value in enumerate(Configuration(sf, tp)
                            for sf in spreading_factors
                            for tp in transmission_powers):
    configurations[idx] = value

# Symbol Rate SF_c [bits/s]
symbol_rates = {}
for idx, configuration in configurations.items():
    if configuration.sf == 7:
        # symbol_rates[idx] = 3417.97 #CR 4
        # symbol_rates[idx] = 4557.29 #CR 3
        # symbol_rates[idx] = 3906.25 #CR 2
        symbol_rates[idx] = 5468.75  #CR 1
    elif configuration.sf == 8:
        # symbol_rates[idx] = 1953.13 #CR 4
        # symbol_rates[idx] = 2232.14 #CR 3
        # symbol_rates[idx] = 2604.17 #CR 2
        symbol_rates[idx] = 3125.00 #CR 1
    elif configuration.sf == 9:
        # symbol_rates[idx] = 1098.63 #CR 4
        # symbol_rates[idx] = 1255.58 #CR 3
        # symbol_rates[idx] = 1464.84 #CR 2
        symbol_rates[idx] = 2197.27 #CR 1
    elif configuration.sf == 10:
        # symbol_rates[idx] = 610.35 #CR 4
        # symbol_rates[idx] = 697.54 #CR 3
        # symbol_rates[idx] = 813.80 #CR 2
        symbol_rates[idx] = 976.56 #CR 1
    elif configuration.sf == 11:
        # symbol_rates[idx] = 335.69  # CR 4
        # symbol_rates[idx] = 383.65  # CR 3
        # symbol_rates[idx] = 447.59  # CR 2
        symbol_rates[idx] = 537.11  # CR 1
    elif configuration.sf == 12:
        # symbol_rates[idx] = 183.11  # CR 4
        # symbol_rates[idx] = 209.26  # CR 3
        # symbol_rates[idx] = 244.14  # CR 2
        symbol_rates[idx] = 292.97  # CR 1

if VERBOSE:
    print('Configurations: ')
    print(configurations)
    print('\nSymbol Rates: ')
    print(symbol_rates)
    print('------------------------------------------------------------------------------')

# -----------------------------
#      Processing PLR'
# -----------------------------

plr1_df = pandas.read_csv(plr1_file, names=['device', 'gateway', 'sf', 'tp'],
                          sep=" ", index_col=False)

inverse_configurations = {value: key for key, value in configurations.items()}
plr1 = defaultdict(list)
for row in plr1_df.itertuples():
    config_id = inverse_configurations[Configuration(row.sf, row.tp)]
    plr1[(row.device, row.gateway)].append(config_id)

if VERBOSE:
    print('PLR\': ')
    print(plr1)
    print('------------------------------------------------------------------------------')

# -----------------------------
# ------ Building Model -------
# -----------------------------

model = pyomo.ConcreteModel()

model.M = pyomo.Set(initialize=gateways.keys())
model.P = pyomo.Set(initialize=gateway_positions.keys())
model.K = pyomo.Set(initialize=end_device_positions.keys())
model.C = pyomo.Set(initialize=configurations.keys())
model.SF = pyomo.Set(initialize=spreading_factors)

model.x = pyomo.Var(model.M, model.P, model.K, model.C, domain=pyomo.Binary)
model.ceil1 = pyomo.Var(model.M, model.P, domain=pyomo.Binary)
model.ceil2 = pyomo.Var(model.M, model.P, domain=pyomo.Binary)
model.ceil3 = pyomo.Var(model.M, model.P, domain=pyomo.Binary)
model.ceil4 = pyomo.Var(model.M, model.SF, domain=pyomo.Binary)

# ------------------
# Objective Function
# ------------------

model.ceil1_lower_bound = pyomo.ConstraintList()
model.ceil1_higher_bound = pyomo.ConstraintList()

expression = 0
for gateway in model.M:
    for position in model.P:
        ceil_expr = sum(
            model.x[gateway, position, device, config] / len(model.K)
            for device in model.K
            for config in model.C
        )
        model.ceil1_lower_bound.add(model.ceil1[gateway, position] - ceil_expr >= 0.0)
        model.ceil1_higher_bound.add(model.ceil1[gateway, position] - ceil_expr <= 1.0 - (1 / len(model.K)))
        expression += model.ceil1[gateway, position]

model.OBJECTIVE = pyomo.Objective(expr=expression, sense=pyomo.minimize)

# ------------------------
# One Position Per Gateway
# ------------------------


# model.ceil2_lower_bound = pyomo.ConstraintList()
# model.ceil2_higher_bound = pyomo.ConstraintList()
# model.single_position_per_gateway = pyomo.ConstraintList()
# for gateway in model.M:
#     expression = 0
#     for position in model.P:
#         ceil_expr = sum(
#             model.x[gateway, position, device, config] / len(model.K)
#             for device in model.K
#             for config in model.C
#         )
#         model.ceil2_lower_bound.add(model.ceil2[gateway, position] - ceil_expr >= 0.0)
#         model.ceil2_higher_bound.add(model.ceil2[gateway, position] - ceil_expr <= 1.0 - (1/len(model.K)))
#         expression += model.ceil2[gateway, position]
#     model.single_position_per_gateway.add(expression <= 1.0)

# ------------------------
# One Gateway Per Position
# ------------------------

model.ceil3_lower_bound = pyomo.ConstraintList()
model.ceil3_higher_bound = pyomo.ConstraintList()
model.single_gateway_per_position = pyomo.ConstraintList()
for position in model.P:
    expression = 0
    for gateway in model.M:
        ceil_expr = sum(
            model.x[gateway, position, device, config] / len(model.K)
            for device in model.K
            for config in model.C
        )
        model.ceil3_lower_bound.add(model.ceil3[gateway, position] - ceil_expr >= 0.0)
        model.ceil3_higher_bound.add(model.ceil3[gateway, position] - ceil_expr <= 1.0 - (1 / len(model.K)))
        expression += model.ceil3[gateway, position]
    model.single_gateway_per_position.add(expression <= 1.0)

# -----------------------------------------------------
# Single Gateway, Position and Configuration per Device
# -----------------------------------------------------

model.single_match_for_device = pyomo.ConstraintList()
for device in model.K:
    expression = sum(
        model.x[gateway, position, device, config]
        for gateway in model.M
        for position in model.P
        for config in model.C
    )
    model.single_match_for_device.add(expression == 1)

# --------------------------
# Gateway Capacity per Slice
# --------------------------

model.gateway_capacity = pyomo.ConstraintList()
for position in model.P:
    for slice in slices.keys():
        expression = sum(
            model.x[gateway, position, device, config] *
            (configurations[config].sf * gateways[gateway].bandwidths[slice] /
             (2 ** configurations[config].sf))
            for device in model.K
            for gateway in model.M
            for config in model.C
            # Equivalent to S(k,l)
            if slice_associations[device] == slice
        )
        for gateway in model.M:
            model.gateway_capacity.add(expression <= gateways[gateway].max_datarates[slice])

# model.gateway_capacity.add(expression <= gateways[gateways].max_datarates[slice])
# ajustar em caso de nº gateways > 1

# --------------
# QoS Constraint
# --------------

model.qos_constraint = pyomo.ConstraintList()
qos_expressions = {}
qos_datarate_expressions = {}
qos_delay_expressions = {}
for gateway in model.M:
    for device in model.K:
        for slice in slices.keys():
            # Equivalent to S(k,l)
            if slice_associations[device] != slice:
                continue

            datarate_expression = sum(
                model.x[gateway, position, device, config] *
                (configurations[config].sf * gateways[gateway].bandwidths[slice] /
                        (2 ** configurations[config].sf)) / MAX_RKC
                for position in model.P
                for config in model.C
            )

            delay_expression = sum(
                model.x[gateway, position, device, config] *
                ( 1 - ( PACKET_SIZE /
                    (configurations[config].sf * gateways[gateway].bandwidths[slice] /
                    (2 ** configurations[config].sf))
                ) / MAX_DELAY )
                for position in model.P
                for config in model.C
            )

            expression = sum(
                model.x[gateway, position, device, config] * (
                    # r_{k,m}
                    (4/5 * configurations[config].sf * gateways[gateway].bandwidths[slice] /
                        (2 ** configurations[config].sf)) / MAX_RKC +
                    # 1 - d_{k,m}
                    ( 1 - (
                        PACKET_SIZE / (configurations[config].sf *
                        gateways[gateway].bandwidths[slice] / (2 ** configurations[config].sf))
                    ) / MAX_DELAY )
                )
                for position in model.P
                for config in model.C
            )

            model.qos_constraint.add(expression >= QOS_LOWER_BOUND)
            qos_expressions[(gateway, device)] = expression
            qos_datarate_expressions[(gateway, device)] = datarate_expression
            qos_delay_expressions[(gateway, device)] = delay_expression

# ---------------
# Loss Constraint
# ---------------
loss_constraints = {}

model.loss_constraint = pyomo.ConstraintList()
for position in model.P:
    for gateway in model.M:
        for device in model.K:
            for config in model.C:
                if not config in plr1[(device, position)]:
                    model.loss_constraint.add(model.x[gateway, position, device, config] == 0)


# model.ceil4_lower_bound = pyomo.ConstraintList()
# model.ceil4_higher_bound = pyomo.ConstraintList()

# for position in model.P:
#     for sf in spreading_factors:
#         ceil_expr = (sum(
#             model.x[gateway, position, device, config]
#             for gateway in model.M
#             for device in model.K
#             for config in model.C
#             if configurations[config].sf == sf
#         ) - 1) / len(model.K)
#         model.ceil4_lower_bound.add(model.ceil4[gateway, sf] - ceil_expr >= 0.0)
#         model.ceil4_higher_bound.add(model.ceil4[gateway, sf] - ceil_expr <= 1.0 - (1 / len(model.K)))

#     for device in model.K:
#         expression = sum(
#             model.x[gateway, position, device, config] * (
#                 # PLR'
#                     (0 if config in plr1[(device, position)] else 1) +
#                     # PLR''
#                     (1 - pyomo.exp(-2 * TRANSMISSION_PROBABILITY *
#                                    # d_{k,m}
#                                    (PACKET_SIZE / (
#                                            configurations[config].sf * gateways[gateway].bandwidths[slice] /
#                                            (2 ** configurations[config].sf)
#                                    )) *
#                                    # number of devices using the same SF
#                                    sum(
#                                        model.x[plr3_gateway, position, plr3_device, plr3_config]
#                                        for plr3_gateway in model.M
#                                        for plr3_device in model.K
#                                        for plr3_config in model.C
#                                        if configurations[plr3_config].sf == configurations[config].sf
#                                    )
#                                    )) +
#                     # PLR'''
#                     model.ceil4[gateway, configurations[config].sf]
#             )
#             for gateway in model.M
#             for config in model.C
#         )

#         # model.loss_constraint.add(expression <= LOSS_HIGHER_BOUND)
#         loss_constraints[(position, device)] = expression

# ---------------
#    Solution
# ---------------

startSolv = timer()
opt = pyomo.SolverFactory('scip')
opt.solve(model, tee=True)

startPos = timer()

gatewaysPositions = ""
devicesConfigurations = ""

for key in model.x:
    if model.x[key].value != 0:
        print(str(model.x[key]), ' -> ', model.x[key].value)

for key in model.x:
    if model.x[key].value != 0:
        print('drone={}, {}, device={}, {}'.format(key[0], gateway_positions[key[1]], key[2], configurations[key[3]]),
              ' -> ', model.x[key].value)
        gatewaysPositions += str(gateway_positions[key[1]].x) + " " + str(gateway_positions[key[1]].y) + " " + str(
            gateway_positions[key[1]].z) + "\n"
        devicesConfigurations += str(key[2]) + " " + str(configurations[key[3]].sf) + " " + str(
            configurations[key[3]].tp) + "\n"

for key in model.ceil1:
    if model.ceil1[key].value != 0:
        print(str(model.ceil1[key]), ' -> ', model.ceil1[key].value)
for key in model.ceil2:
    if model.ceil1[key].value != 0:
        print(str(model.ceil2[key]), ' -> ', model.ceil2[key].value)
for key in model.ceil3:
    if model.ceil1[key].value != 0:
        print(str(model.ceil3[key]), ' -> ', model.ceil3[key].value)
for key in model.ceil4:
    if model.ceil4[key].value != 0:
        print(str(model.ceil4[key]), ' -> ', model.ceil4[key].value)

# lossResults = ""
# for gateway, expr in loss_constraints.items():
#     if pyomo.value(expr) != 0:
#         print('Loss Constraint (GatewayPosition={}, Device={}) -> '.format(gateway[0], gateway[1]), pyomo.value(expr))
#         lossResults += str(gateway[0]) + " " + str(gateway[1]) + " " + str(pyomo.value(expr)) + "\n"

qosResults = ""
delayResults = ""
drResults = ""
for key in qos_expressions.keys():
    print('QoS Gateway={} Device={} -> '.format(key[0], key[1]), pyomo.value(qos_expressions[key]))
    print('      Datarate QoS -> '.format(key[0], key[1]), pyomo.value(qos_datarate_expressions[key]))
    print('         Delay QoS -> '.format(key[0], key[1]), pyomo.value(qos_delay_expressions[key]))
    qosResults += str(key[0]) + " " + str(key[1]) + " " + str(pyomo.value(qos_expressions[key])) + "\n"
    drResults += str(key[0]) + " " + str(key[1]) + " " + str(pyomo.value(qos_datarate_expressions[key])) + "\n"
    delayResults += str(key[0]) + " " + str(key[1]) + " " + str(pyomo.value(qos_delay_expressions[key])) + "\n"

fileGwPlacement =  path_output + "optimizedPlacement_"  + \
                                str(args.seed) + "s_" + \
                                str(args.nGat) + "x" + \
                                str(args.nPla) +"Gv_" + \
                                str(args.nDev) + "D.dat"

data = pandas.read_csv(io.StringIO(gatewaysPositions), names=['x', 'y', 'z'], sep=" ", index_col=False)
data.drop_duplicates(inplace=True)
data.to_csv(fileGwPlacement, header=False, sep=" ", index=False)

fileCfgPlacement =  path_output + "optimizedDevicesConfigurations_" + \
                                str(args.seed) + "s_" + \
                                str(args.nGat) + "x" + \
                                str(args.nPla) +"Gv_" + \
                                str(args.nDev) + "D.dat"
with open(fileCfgPlacement, "w+") as outfile:
    outfile.write(devicesConfigurations)

# fileLoss = path_output + "optimizedLoosResults"  + \
#                          str(args.seed) + "s_" + \
#                          str(args.nGat) + "x" + \
#                          str(args.nPla) +"Gv_" + \
#                          str(args.nDev) + "D.dat"
# with open(fileLoss, "w+") as outfile:
#     outfile.write(lossResults)

fileQoS = path_output + "optimizedQoSResults_" + \
                         str(args.seed) + "s_" + \
                         str(args.nGat) + "x" + \
                         str(args.nPla) +"Gv_" + \
                         str(args.nDev) + "D.dat"
with open(fileQoS, "w+") as outfile:
    outfile.write(qosResults)

fileDR = path_output + "optimizedDR_Results_" + \
                         str(args.seed) + "s_" + \
                         str(args.nGat) + "x" + \
                         str(args.nPla) +"Gv_" + \
                         str(args.nDev) + "D.dat"
with open(fileDR, "w+") as outfile:
    outfile.write(drResults)

fileDelay = path_output + "optimizedDelayResults_" + \
                         str(args.seed) + "s_" + \
                         str(args.nGat) + "x" + \
                         str(args.nPla) +"Gv_" + \
                         str(args.nDev) + "D.dat"
with open(fileDelay, "w+") as outfile:
    outfile.write(delayResults)

#DEBUG Output filenames
if DEBUG:
    print(fileGwPlacement)
    print(fileCfgPlacement)
    print(fileDelay)
    print(fileDR)
    print(fileQoS)

endPos = timer()

print('Pre processing elapsed time:' + str(startSolv - startPre) + 'sec')
print('Processing elapsed time:' + str(startPos - startSolv) + 'sec')
print('Pre processing elapsed time:' + str(endPos - startPos) + 'sec')
print('Elapsed time:' + str(endPos - startPre) + 'sec')

os.system("date")
