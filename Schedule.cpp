#include "Schedule.h"

// Глобальный объект, предоставляющий ф-ии для работы с расписанием
CSchedule* Schedule;

STimeBlock::STimeBlock()
{
	bType = TIME_BLOCK_NONE;
	bUseTo = false;
	bUseRepeat = false;
	wRepeatMin = 1;

	bWeekNumber = 1;
	bWeekNumberTo = 1;

	bDayDayOfWeek = 1;
	bDayDayOfWeekTo = 1;

	bHourHour = 0;
	bHourHourTo = 0;

	bMinExecute = 0;
}

///////////////////////////////////////////////////////////////////////////////
// CSchedule
///////////////////////////////////////////////////////////////////////////////
CSchedule::CSchedule()
{
}

CSchedule::~CSchedule()
{
}

WORD CSchedule::GetNextID(CString& sSource, WORD wStart, CString& sID, CString& sValue)
{
	for (WORD i = wStart; i < sSource.Length(); i++)
		if (sSource[i] == '(')
		{
			sID = sSource.SubStr(wStart, i - wStart);
			wStart = i + 1;
		}
		else if (sSource[i] == ')')
		{
			sValue = sSource.SubStr(wStart, i - wStart);
			break;
		}

	return i + 1;
}

bool CSchedule::GetTwoNums(CString sSource, int& iOne, int& iTwo)
{
	// Распознаёт в строке одно\два числа
	// '1-2' - вернёт true и iOne=1, iTwo=2
	// '1' - вернёт false и iOne=1

	WORD wDelPos = (WORD) -1;
	for (WORD i = 0; i < sSource.Length(); i++)
		if (sSource[i] == '-')
		{
			wDelPos = i;
			break;
		}

	if (wDelPos == (WORD) - 1)
	{
		iOne = sSource.ToInt();
		iTwo = 0;
		return false;
	}
	else
	{
		iOne = sSource.SubStr(0, wDelPos).ToInt();
		iTwo = sSource.SubStr(wDelPos + 1, sSource.Length() - wDelPos - 1).ToInt();
		return true;
	}
}

CString CSchedule::MakePeriod(BYTE bFirst, BYTE bSecond, bool bFromTo)
{
	if (bFromTo)
        return FormatC(TEXT("P(%d-%d)"), bFirst, bSecond);
	else
		return FormatC(TEXT("P(%d)"), bFirst);
}

PTBTreeItem CSchedule::GetActiveTB(PTBTreeItem pScheduleTree, CDateTime dTime)
{
	PTBTreeItem ptiActive = pScheduleTree;
	if (ptiActive == NULL) return NULL;

	PTBTreeItem res = NULL;

	while (ptiActive->aChilds.Size() > 0)
	{
		bool bBreak = true;

		for (BYTE i = 0; i < ptiActive->aChilds.Size(); i++)
		{
			PTBTreeItem ptic = ptiActive->aChilds[i];

			// Выясним, удовлетворяет ли текущий ВБ текущей дате
			switch (ptic->TimeBlock.bType)
			{
			case TIME_BLOCK_DAY_CHILD:
				{
					if (ptic->TimeBlock.bUseTo)
					{
						if (dTime.GetPSystemTime()->wDayOfWeek >= ptic->TimeBlock.bDayDayOfWeek && 
							(dTime.GetPSystemTime()->wDayOfWeek == 0 ? 7 <= ptic->TimeBlock.bDayDayOfWeekTo : dTime.GetPSystemTime()->wDayOfWeek <= ptic->TimeBlock.bDayDayOfWeekTo))
						{
							ptiActive = ptic;
							bBreak = false;
							break;
						}
					}
					else if (dTime.GetPSystemTime()->wDayOfWeek == ptic->TimeBlock.bDayDayOfWeek)
					{
						ptiActive = ptic;
						bBreak = false;
						break;
					}
				}
				break;

			case TIME_BLOCK_HOUR_CHILD:
				{
					if (ptic->TimeBlock.bUseTo)
					{
						if (dTime.GetPSystemTime()->wHour >= ptic->TimeBlock.bHourHour && dTime.GetPSystemTime()->wHour <= ptic->TimeBlock.bHourHourTo)
						{
							res = ptic;
							ptiActive = ptic;
							bBreak = false;
							break;
						}
					}
					else if (dTime.GetPSystemTime()->wHour == ptic->TimeBlock.bHourHour)
					{
						res = ptic;
						ptiActive = ptic;
						bBreak = false;
						break;
					}
				}
				break;
			}
		}

		if (bBreak) break;
	}

	if (ptiActive->TimeBlock.bType == TIME_BLOCK_HOUR)
		res = ptiActive;

	return res;
}

PTBTreeItem CSchedule::GetNextSibling_(PTBTreeItem pItem)
{
	// Получим след. ребёнка одного уровня с pItem
	if (pItem == NULL || pItem->pParent == NULL)
		return NULL;

	CList<PTBTreeItem>* pSiblings = &pItem->pParent->aChilds;

	for (int i = 0; i < pSiblings->Size(); i++)
	{
		// в списке дейтей нашли себя (т.е. нашли pItem)
		if (pSiblings->operator[](i) == pItem)
			// вернём след. сиблинга, если есть
			if (pSiblings->Size() > i + 1)
			{
				return pSiblings->operator[](i + 1);
				break;
			}
	}

	return NULL;
}

PTBTreeItem CSchedule::BuildScheduleTree(CString sTree)
{
	sTree.ToLower();

	CString sID(3), sParam(20);
	WORD wPos = 0;

	PTBTreeItem res = NULL;
	PTBTreeItem pTBMonthParent = NULL;
	PTBTreeItem pTBWeekParent = NULL;
	PTBTreeItem pTBDayParent = NULL;

	PTBTreeItem ptbCur = NULL;
	
	do
	{
		wPos = GetNextID(sTree, wPos, sID, sParam);

		// если идентификатор - "временной блок"
		if (sID == TEXT("tb"))
		{
			PTBTreeItem ptb = new STBTreeItem;
			ptbCur = ptb;

			if (sParam == TEXT("month"))
			{
				ptb->TimeBlock.bType = TIME_BLOCK_MONTH;
				pTBMonthParent = ptb;

				if (res == NULL) res = ptb;
			}
			else if (sParam == TEXT("week"))
			{
				ptb->TimeBlock.bType = (pTBMonthParent == NULL ? TIME_BLOCK_WEEK : TIME_BLOCK_WEEK_CHILD);
				ptb->pParent = pTBMonthParent;
				pTBWeekParent = ptb;

				if (pTBMonthParent) pTBMonthParent->aChilds.Add(pTBWeekParent);
				else if (res == NULL) res = ptb;
			}
			else if (sParam == TEXT("day"))
			{
				ptb->TimeBlock.bType = (pTBWeekParent == NULL ? TIME_BLOCK_DAY : TIME_BLOCK_DAY_CHILD);
				ptb->pParent = pTBWeekParent;
				pTBDayParent = ptb;

				if (pTBWeekParent) pTBWeekParent->aChilds.Add(pTBDayParent);
				else if (res == NULL) res = ptb;
			}
			else if (sParam == TEXT("hour"))
			{
				ptb->TimeBlock.bType = (pTBDayParent == NULL ? TIME_BLOCK_HOUR : TIME_BLOCK_HOUR_CHILD);
				ptb->pParent = pTBDayParent;

				if (pTBDayParent) pTBDayParent->aChilds.Add(ptb);
				else if (res == NULL) res = ptb;
			}
		}
		// если идентификатор - "период"
		else if (sID == TEXT("p") && ptbCur)
		{
			int iOne, iTwo;
			ptbCur->TimeBlock.bUseTo = GetTwoNums(sParam, iOne, iTwo);

			switch (ptbCur->TimeBlock.bType)
			{
			case TIME_BLOCK_WEEK_CHILD: 
				{
					ptbCur->TimeBlock.bWeekNumber = iOne;
					ptbCur->TimeBlock.bWeekNumberTo = iTwo;
				}
				break;

			case TIME_BLOCK_DAY_CHILD: 
				{
					//if (iOne == 7) iOne = 0;
					//if (iTwo == 7) iTwo = 0;

					ptbCur->TimeBlock.bDayDayOfWeek = iOne;
					ptbCur->TimeBlock.bDayDayOfWeekTo = iTwo;
				}
				break;

			case TIME_BLOCK_HOUR_CHILD: 
				{
					ptbCur->TimeBlock.bHourHour = iOne;
					ptbCur->TimeBlock.bHourHourTo = iTwo;
				}
				break;
			}
		}
		// если идентификатор - "время начала выполнения (мин)"
		else if (sID == TEXT("s") && ptbCur)
		{
			ptbCur->TimeBlock.bMinExecute = sParam.ToInt();
		}
		// если идентификатор - "повторение"
		else if (sID == TEXT("r") && ptbCur)
		{
			ptbCur->TimeBlock.bUseRepeat = true;
			ptbCur->TimeBlock.wRepeatMin = sParam.ToInt();
		}
	} while (wPos < sTree.Length());

	return res;
}

CString CSchedule::ScheduleTreeToString(PTBTreeItem pRoot)
{
	CString res;
	PTBTreeItem pCur = pRoot;

	while (pCur != NULL)
	{
		PTimeBlock ptb = &pCur->TimeBlock;

		switch (ptb->bType) 
		{
		case TIME_BLOCK_MONTH: 
			{
				res += TEXT("TB(Month)");
			}
			break;

		case TIME_BLOCK_WEEK: 
		case TIME_BLOCK_WEEK_CHILD: 
			{
				res += TEXT("TB(Week)");
				res += MakePeriod(ptb->bWeekNumber, ptb->bWeekNumberTo, ptb->bUseTo);
			}
			break;

		case TIME_BLOCK_DAY: 
		case TIME_BLOCK_DAY_CHILD: 
			{
				res += TEXT("TB(Day)");
				res += MakePeriod(ptb->bDayDayOfWeek, ptb->bDayDayOfWeekTo, ptb->bUseTo);
			}
			break;

		case TIME_BLOCK_HOUR:
		case TIME_BLOCK_HOUR_CHILD: 
			{
				res += TEXT("TB(Hour)");
				res += MakePeriod(ptb->bHourHour, ptb->bHourHourTo, ptb->bUseTo);
				res += CString(FormatC(TEXT("S(%d)"), ptb->bMinExecute));
				if (ptb->bUseRepeat) res += CString(FormatC(TEXT("R(%d)"), ptb->wRepeatMin));
			}
			break;
		}

		// Вычисляем следующий эл-т как если бы дерево было плоским
		if (pCur->aChilds.Size())
		{
			// если есть дети - возвращаем первого ребёнка
			pCur = pCur->aChilds[0];
		}
		else
		{
			// Получим след. ребёнка одного уровня с текущим эл-м
			PTBTreeItem pNext = GetNextSibling_(pCur);

			if (pNext)
			{
				pCur = pNext;
			}
			// нет след. сиблинга, ищем сиблинга более высоких уровней
			else
			{
				PTBTreeItem pParent = pCur;

				do
				{
					pParent = pParent->pParent;
					pCur = GetNextSibling_(pParent);
				} while (pCur == NULL && pParent != NULL);
			}
		}
	}

	return res;
}

bool CSchedule::FreeScheduleTree(PTBTreeItem pRoot)
{
	if (pRoot->aChilds.Size())
	{
		// Для всех детей вызываемся рекурсивно
		for (BYTE i = 0; i < pRoot->aChilds.Size(); i++)
			FreeScheduleTree(pRoot->aChilds[i]);
	}
	else
		// Детей нет - удаляемся
		delete pRoot;

	return true;
}

CString CSchedule::TimeBlockToString(PTimeBlock pTimeBlock)
{
	switch (pTimeBlock->bType)
	{
	case TIME_BLOCK_MONTH: return TEXT("Месяц"); break;
	
	case TIME_BLOCK_WEEK: return TEXT("Неделя"); break;
	case TIME_BLOCK_WEEK_CHILD: 
		{
			if (pTimeBlock->bUseTo == 0)
				return FormatC(TEXT("Неделя %d"), pTimeBlock->bWeekNumber);
			else
				return FormatC(TEXT("Неделя с %d по %d"), pTimeBlock->bWeekNumber, pTimeBlock->bWeekNumberTo);
		}

	case TIME_BLOCK_DAY: return TEXT("День"); break;
	case TIME_BLOCK_DAY_CHILD: 
		{
			if (pTimeBlock->bUseTo == 0)
				return FormatC(TEXT("День - %s"), DayOfWeekToStr(pTimeBlock->bDayDayOfWeek).C());
			else
				return FormatC(TEXT("День с %s по %s"), DayOfWeekToStr(pTimeBlock->bDayDayOfWeek).C(), DayOfWeekToStr(pTimeBlock->bDayDayOfWeekTo).C());
		}

	case TIME_BLOCK_HOUR: return TEXT("Час"); break;
	case TIME_BLOCK_HOUR_CHILD:	
		{
			CString res;
			if (pTimeBlock->bUseTo == 0)
				res = FormatC(TEXT("Час - %d:%.2d"), pTimeBlock->bHourHour, pTimeBlock->bMinExecute); 
			else
				res = FormatC(TEXT("Час с %d:%.2d по %d:%.2d"), pTimeBlock->bHourHour, pTimeBlock->bMinExecute,
					pTimeBlock->bHourHourTo, pTimeBlock->bMinExecute); 

			if (pTimeBlock->bUseRepeat)
			{
				res += CString(FormatC(TEXT(", каждые %d мин"), pTimeBlock->wRepeatMin));
			}

			return res;
		}
		
	default: return TEXT("");
	}
}

CDateTime CSchedule::GetNextRun(PTBTreeItem pScheduleTree, CDateTime dFrom)
{
	CDateTime dtNow(dFrom);
	CDateTime res;
	CDateTime dt;

	bool bFirst = true;
	bool bBreak = false;
	PTBTreeItem pti = NULL;
	DWORD dwHour = 0;

	while (!bBreak)
	{
		pti = GetActiveTB(pScheduleTree, dt);	

		if (pti == NULL && dwHour < 24 * 7) 
		{
			dt.GetPSystemTime()->wMinute = 0;
			dt += DT_HOUR;
			bFirst = false;
			dwHour++;

			continue;
		}

		if (!pti)
			bBreak = true;
		else if (pti)
		{
			if (bFirst)
			{
				if (dtNow.GetPSystemTime()->wMinute >= pti->TimeBlock.bMinExecute)
				{
					if (pti->TimeBlock.bUseRepeat)
					{
						CDateTime dtPeriodStart;
						dtPeriodStart.GetPSystemTime()->wHour = pti->TimeBlock.bHourHour;
						dtPeriodStart.GetPSystemTime()->wMinute = pti->TimeBlock.bMinExecute;
						  
						DWORD wMin = dtPeriodStart.DeltaMin(dtNow);
						wMin = (wMin / pti->TimeBlock.wRepeatMin + 1) * pti->TimeBlock.wRepeatMin;

						res = dtPeriodStart + (__int64) wMin * DT_MINUTE;
						dt = res;

						bFirst = false;
					}
					else
					{
						dt.GetPSystemTime()->wMinute = pti->TimeBlock.bMinExecute;
						dt += DT_HOUR;

						bFirst = false;
						dwHour++;
					}
				}
				else 
				{
					res = dt;
					res.GetPSystemTime()->wMinute = pti->TimeBlock.bMinExecute;
					dt = res;
				}

				bFirst = false;
			}
			else if (!bFirst)
			{
				res = dt;
				bBreak = true;
			}
		}
	}    

	if (pti == NULL) 
	{
		SYSTEMTIME stNULL = {0};
		return CDateTime(stNULL);
	}

	res.GetPSystemTime()->wSecond = 0;
	res.GetPSystemTime()->wMilliseconds = 0;

	return res;
}

__int64 CSchedule::GetNextRunMSec(CDateTime dtRun)
{
	return DTNow().DeltaMSec(dtRun);
}

int CSchedule::GetTimeBlockMaxChilds(BYTE bTBType)
{
	switch (bTBType)
	{
	case TIME_BLOCK_MONTH: return 4; break;
	case TIME_BLOCK_WEEK:
	case TIME_BLOCK_WEEK_CHILD: return 7; break;
	case TIME_BLOCK_DAY:
	case TIME_BLOCK_DAY_CHILD: return 24; break;
	case TIME_BLOCK_HOUR:
	case TIME_BLOCK_HOUR_CHILD: return 0; break;
	default: return 1;
	}
}