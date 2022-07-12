import sys
from yavl import CompilerOptions, CodeGenerator

class MyCustomCodeGenerator(CodeGenerator):
    
    def emit_map_declaration(self, type_name, type_info):
        self.writeln("typedef struct {} {{".format(type_name))
        for field_name, field_type in type_info.items():
            bracket_pos = field_type.find("[")
            is_array_type = bracket_pos != -1
            if is_array_type:
                base_type = field_type[0:bracket_pos]
                array_length = field_type[bracket_pos:]
                self.writeln("{} {}{};".format(base_type, field_name, array_length))
            else:
                self.writeln("{} {};".format(field_type, field_name))
        self.writeln("}} {};".format(type_name))
        self.writeln()
