		case <statement_enum_type>: {
			ExpressionStatus status[<num_param>];
			ParameterList* paramList = stmt->parameters;
			#include: parameter_fetch
		
			#include: parameter_assert
		
			#include: module_call
			break;
		} 
		
