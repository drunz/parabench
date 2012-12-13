			IOStatus ioStatus = <io_func_call>(<io_func_param_list>);
			dump_coretime(coreTimeStack, ioStatus.coreTime);
			if (ioStatus.success)
				statementsSucceed[<statement_enum_type>]++;
			else
				statementsFail[<statement_enum_type>]++;
