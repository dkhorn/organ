import mido
print("INPUTS:")
for n in mido.get_input_names():
    print(" ", n)
print("OUTPUTS:")
for n in mido.get_output_names():
    print(" ", n)
