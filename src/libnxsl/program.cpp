/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: program.cpp
**
**/

#include "libnxsl.h"


//
// Constants
//

#define MAX_ERROR_NUMBER         8
#define CONTROL_STACK_LIMIT      32768


//
// Command mnemonics
//

static char *m_szCommandMnemonic[] =
{
   "NOP", "RET", "JMP", "CALL", "CALL",
   "PUSH", "PUSH", "EXIT", "POP", "SET",
   "ADD", "SUB", "MUL", "DIV", "REM",
   "EQ", "NE", "LT", "LE", "GT", "GE",
   "BITAND", "BITOR", "BITXOR",
   "AND", "OR", "LSHIFT", "RSHIFT",
   "NRET", "JZ", "PRINT", "CONCAT",
   "BIND"
};


//
// Error texts
//

static TCHAR *m_szErrorMessage[MAX_ERROR_NUMBER] =
{
   _T("Data stack underflow"),
   _T("Control stack underflow"),
   _T("Condition value is not a number"),
   _T("Bad arithmetic conversion"),
   _T("Invalid operation with NULL value"),
   _T("Internal error"),
   _T("main() function not presented"),
   _T("Control stack overflow")
};


//
// Constructor
//

NXSL_Program::NXSL_Program(void)
{
   m_ppInstructionSet = NULL;
   m_dwCodeSize = 0;
   m_dwCurrPos = INVALID_ADDRESS;
   m_pDataStack = NULL;
   m_pCodeStack = NULL;
   m_nErrorCode = 0;
   m_pszErrorText = NULL;
   m_pConstants = new NXSL_VariableSystem;
   m_pGlobals = new NXSL_VariableSystem;
   m_pLocals = NULL;
   m_dwNumFunctions = 0;
   m_pFunctionList = NULL;
   m_dwSubLevel = 0;    // Level of current subroutine
}


//
// Destructor
//

NXSL_Program::~NXSL_Program(void)
{
   DWORD i;

   for(i = 0; i < m_dwCodeSize; i++)
      delete m_ppInstructionSet[i];
   safe_free(m_ppInstructionSet);
   
   delete m_pDataStack;
   delete m_pCodeStack;

   delete m_pConstants;
   delete m_pGlobals;
   delete m_pLocals;

   safe_free(m_pFunctionList);

   safe_free(m_pszErrorText);
}


//
// Add new instruction to set
//

void NXSL_Program::AddInstruction(NXSL_Instruction *pInstruction)
{
   m_ppInstructionSet = (NXSL_Instruction **)realloc(m_ppInstructionSet,
         sizeof(NXSL_Instruction *) * (m_dwCodeSize + 1));
   m_ppInstructionSet[m_dwCodeSize++] = pInstruction;
}


//
// Resolve last jump with INVALID_ADDRESS to current address
//

void NXSL_Program::ResolveLastJump(int nOpCode)
{
   DWORD i;

   for(i = m_dwCodeSize; i > 0;)
   {
      i--;
      if ((m_ppInstructionSet[i]->m_nOpCode == nOpCode) &&
          (m_ppInstructionSet[i]->m_operand.m_dwAddr == INVALID_ADDRESS))
      {
         m_ppInstructionSet[i]->m_operand.m_dwAddr = m_dwCodeSize;
         break;
      }
   }
}


//
// Add new function to defined functions list
// Will use first free address if dwAddr == INVALID_ADDRESS
//

BOOL NXSL_Program::AddFunction(char *pszName, DWORD dwAddr, char *pszError)
{
   DWORD i;

   // Check for duplicate function names
   for(i = 0; i < m_dwNumFunctions; i++)
      if (!strcmp(m_pFunctionList[i].m_szName, pszName))
      {
         sprintf(pszError, "Duplicate function name: \"%s\"", pszName);
         return FALSE;
      }
   m_dwNumFunctions++;
   m_pFunctionList = (NXSL_Function *)realloc(m_pFunctionList, sizeof(NXSL_Function) * m_dwNumFunctions);
   nx_strncpy(m_pFunctionList[i].m_szName, pszName, MAX_FUNCTION_NAME);
   m_pFunctionList[i].m_dwAddr = (dwAddr == INVALID_ADDRESS) ? m_dwCodeSize : dwAddr;
   return TRUE;
}


//
// resolve local functions
//

void NXSL_Program::ResolveFunctions(void)
{
   DWORD i, j;

   for(i = 0; i < m_dwCodeSize; i++)
   {
      if (m_ppInstructionSet[i]->m_nOpCode == OPCODE_CALL_EXTERNAL)
      {
         for(j = 0; j < m_dwNumFunctions; j++)
         {
            if (!strcmp(m_pFunctionList[j].m_szName,
                        m_ppInstructionSet[i]->m_operand.m_pszString))
            {
               free(m_ppInstructionSet[i]->m_operand.m_pszString);
               m_ppInstructionSet[i]->m_operand.m_dwAddr = m_pFunctionList[j].m_dwAddr;
               m_ppInstructionSet[i]->m_nOpCode = OPCODE_CALL;
               break;
            }
         }
      }
   }
}


//
// Dump program to file (as text)
//

void NXSL_Program::Dump(FILE *pFile)
{
   DWORD i;

   for(i = 0; i < m_dwCodeSize; i++)
   {
      fprintf(pFile, "%04X  %-6s  ", i,
              m_szCommandMnemonic[m_ppInstructionSet[i]->m_nOpCode]);
      switch(m_ppInstructionSet[i]->m_nOpCode)
      {
         case OPCODE_CALL_EXTERNAL:
            fprintf(pFile, "%s, %d\n", m_ppInstructionSet[i]->m_operand.m_pszString,
                    m_ppInstructionSet[i]->m_nStackItems);
            break;
         case OPCODE_CALL:
            fprintf(pFile, "%04X, %d\n", m_ppInstructionSet[i]->m_operand.m_dwAddr,
                    m_ppInstructionSet[i]->m_nStackItems);
            break;
         case OPCODE_JMP:
         case OPCODE_JZ:
            fprintf(pFile, "%04X\n", m_ppInstructionSet[i]->m_operand.m_dwAddr);
            break;
         case OPCODE_PUSH_VARIABLE:
         case OPCODE_SET:
         case OPCODE_BIND:
            fprintf(pFile, "%s\n", m_ppInstructionSet[i]->m_operand.m_pszString);
            break;
         case OPCODE_PUSH_CONSTANT:
            if (m_ppInstructionSet[i]->m_operand.m_pConstant->IsNull())
               fprintf(pFile, "<null>\n");
            else
               fprintf(pFile, "\"%s\"\n", 
                       m_ppInstructionSet[i]->m_operand.m_pConstant->GetValueAsString());
            break;
         case OPCODE_POP:
            fprintf(pFile, "%d\n", m_ppInstructionSet[i]->m_nStackItems);
            break;
         default:
            fprintf(pFile, "\n");
            break;
      }
   }
}


//
// Report error
//

void NXSL_Program::Error(int nError)
{
   TCHAR szBuffer[1024];

   safe_free(m_pszErrorText);
   _sntprintf(szBuffer, 1024, _T("Error %d in line %d: %s"), nError,
              (m_dwCurrPos == INVALID_ADDRESS) ? 0 : m_ppInstructionSet[m_dwCurrPos]->m_nSourceLine,
              ((nError > 0) && (nError <= MAX_ERROR_NUMBER)) ? m_szErrorMessage[nError - 1] : _T("Unknown error code"));
   m_pszErrorText = _tcsdup(szBuffer);
   m_dwCurrPos = INVALID_ADDRESS;
}


//
// Run program
//

int NXSL_Program::Run(void)
{
   DWORD i;

   // Create stacks
   m_pDataStack = new NXSL_Stack;
   m_pCodeStack = new NXSL_Stack;

   // Create local variable system for main()
   m_pLocals = new NXSL_VariableSystem;

   // Locate main()
   for(i = 0; i < m_dwNumFunctions; i++)
      if (!strcmp(m_pFunctionList[i].m_szName, "main"))
         break;
   if (i < m_dwNumFunctions)
   {
      m_dwCurrPos = m_pFunctionList[i].m_dwAddr;
      while(m_dwCurrPos < m_dwCodeSize)
         Execute();
   }
   else
   {
      Error(7);
      m_dwCurrPos = INVALID_ADDRESS;
   }

   return (m_dwCurrPos == INVALID_ADDRESS) ? -1 : 0;
}


//
// Find variable or create if does not exist
//

NXSL_Variable *NXSL_Program::FindOrCreateVariable(TCHAR *pszName)
{
   NXSL_Variable *pVar;

   pVar = m_pConstants->Find(pszName);
   if (pVar == NULL)
   {
      pVar = m_pGlobals->Find(pszName);
      if (pVar == NULL)
      {
         pVar = m_pLocals->Find(pszName);
         if (pVar == NULL)
         {
            pVar = m_pLocals->Create(pszName);
         }
      }
   }
   return pVar;
}


//
// Execute single instruction
//

void NXSL_Program::Execute(void)
{
   NXSL_Instruction *cp;
   NXSL_Value *pValue;
   NXSL_Variable *pVar;
   DWORD dwNext = m_dwCurrPos + 1;
   char szBuffer[256];
   int i;

   cp = m_ppInstructionSet[m_dwCurrPos];
   switch(cp->m_nOpCode)
   {
      case OPCODE_PUSH_CONSTANT:
         m_pDataStack->Push(new NXSL_Value(cp->m_operand.m_pConstant));
         break;
      case OPCODE_PUSH_VARIABLE:
         pVar = FindOrCreateVariable(cp->m_operand.m_pszString);
         m_pDataStack->Push(new NXSL_Value(pVar->Value()));
         break;
      case OPCODE_SET:
         pVar = FindOrCreateVariable(cp->m_operand.m_pszString);
         pValue = (NXSL_Value *)m_pDataStack->Peek();
         if (pValue != NULL)
         {
            pVar->Set(new NXSL_Value(pValue));
         }
         else
         {
            Error(1);
         }
         break;
      case OPCODE_POP:
         for(i = 0; i < cp->m_nStackItems; i++)
            delete (NXSL_Value *)m_pDataStack->Pop();
         break;
      case OPCODE_JMP:
         dwNext = cp->m_operand.m_dwAddr;
         break;
      case OPCODE_JZ:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            if (pValue->IsNumeric())
            {
               if (pValue->GetValueAsInt() == 0)
                  dwNext = cp->m_operand.m_dwAddr;
            }
            else
            {
               Error(3);
            }
            delete pValue;
         }
         else
         {
            Error(1);
         }
         break;
      case OPCODE_CALL:
         if (m_dwSubLevel < CONTROL_STACK_LIMIT)
         {
            m_dwSubLevel++;
            dwNext = cp->m_operand.m_dwAddr;
            m_pCodeStack->Push((void *)(m_dwCurrPos + 1));
            m_pCodeStack->Push(m_pLocals);
            m_pLocals = new NXSL_VariableSystem;
            m_nBindPos = 1;

            // Bind arguments
            for(i = cp->m_nStackItems; i > 0; i--)
            {
               pValue = (NXSL_Value *)m_pDataStack->Pop();
               if (pValue != NULL)
               {
                  sprintf(szBuffer, "$%d", i);
                  m_pLocals->Create(szBuffer, pValue);
               }
               else
               {
                  Error(1);
                  break;
               }
            }
         }
         else
         {
            Error(8);
         }
         break;
      case OPCODE_RET_NULL:
         m_pDataStack->Push(new NXSL_Value);
      case OPCODE_RETURN:
         if (m_dwSubLevel > 0)
         {
            m_dwSubLevel--;
            delete m_pLocals;
            m_pLocals = (NXSL_VariableSystem *)m_pCodeStack->Pop();
            dwNext = (DWORD)m_pCodeStack->Pop();
         }
         else
         {
            // Return from main(), terminate program
            dwNext = m_dwCodeSize;
         }
         break;
      case OPCODE_BIND:
         sprintf(szBuffer, "$%d", m_nBindPos++);
         pVar = m_pLocals->Find(szBuffer);
         pValue = (pVar != NULL) ? new NXSL_Value(pVar->Value()) : new NXSL_Value;
         pVar = m_pLocals->Find(cp->m_operand.m_pszString);
         if (pVar == NULL)
            m_pLocals->Create(cp->m_operand.m_pszString, pValue);
         else
            pVar->Set(pValue);
         break;
      case OPCODE_PRINT:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            fputs(pValue->GetValueAsString(), stdout);
            delete pValue;
         }
         else
         {
            Error(1);
         }
         break;
      case OPCODE_EXIT:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            dwNext = m_dwCodeSize;
            delete pValue;
         }
         else
         {
            Error(1);
         }
         break;
      case OPCODE_ADD:
      case OPCODE_SUB:
      case OPCODE_MUL:
      case OPCODE_DIV:
      case OPCODE_REM:
      case OPCODE_CONCAT:
      case OPCODE_EQ:
      case OPCODE_NE:
      case OPCODE_LT:
      case OPCODE_LE:
      case OPCODE_GT:
      case OPCODE_GE:
         DoBinaryOperation(cp->m_nOpCode);
         break;
      default:
         break;
   }

   if (m_dwCurrPos != INVALID_ADDRESS)
      m_dwCurrPos = dwNext;
}


//
// Perform binary operation on two operands from stack and push result to stack
//

void NXSL_Program::DoBinaryOperation(int nOpCode)
{
   NXSL_Value *pVal1, *pVal2, *pRes = NULL;
   int nResult;

   pVal2 = (NXSL_Value *)m_pDataStack->Pop();
   pVal1 = (NXSL_Value *)m_pDataStack->Pop();

   if ((pVal1 != NULL) && (pVal2 != NULL))
   {
      if ((!pVal1->IsNull() && !pVal2->IsNull()) ||
          (nOpCode == OPCODE_EQ) || (nOpCode == OPCODE_NE))
      {
         if (pVal1->IsNumeric() && pVal2->IsNumeric())
         {
            switch(nOpCode)
            {
               case OPCODE_CONCAT:
                  pRes = pVal1;
                  pRes->Concatenate(pVal2->GetValueAsString());
                  delete pVal2;
                  break;
               default:
                  Error(6);
                  break;
            }
         }
         else
         {
            switch(nOpCode)
            {
               case OPCODE_EQ:
               case OPCODE_NE:
                  if (pVal1->IsNull() && pVal2->IsNull())
                  {
                     nResult = 1;
                  }
                  else if (pVal1->IsNull() || pVal2->IsNull())
                  {
                     nResult = 0;
                  }
                  else
                  {
                     nResult = !strcmp(pVal1->GetValueAsString(), pVal2->GetValueAsString());
                  }
                  delete pVal1;
                  delete pVal2;
                  pRes = new NXSL_Value((nOpCode == OPCODE_EQ) ? nResult : !nResult);
                  break;
               case OPCODE_CONCAT:
                  if (pVal1->IsNull() || pVal2->IsNull())
                  {
                     Error(5);
                  }
                  else
                  {
                     pRes = pVal1;
                     pRes->Concatenate(pVal2->GetValueAsString());
                     delete pVal2;
                  }
                  break;
               default:
                  Error(6);
                  break;
            }
         }
      }
      else
      {
         Error(5);
      }
   }
   else
   {
      Error(1);
   }

   if (pRes != NULL)
      m_pDataStack->Push(pRes);
}
