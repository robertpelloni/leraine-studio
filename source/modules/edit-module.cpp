#include "edit-module.h"

bool EditModule::OnMouseLeftButtonReleased()
{
	return _EditModes[_SelectedEditMode]->OnMouseLeftButtonReleased();
}

bool EditModule::OnMouseRightButtonClicked(const bool InIsShiftDown)
{
	return _EditModes[_SelectedEditMode]->OnMouseRightButtonClicked(InIsShiftDown);
}

bool EditModule::OnMouseRightButtonReleased()
{
	return _EditModes[_SelectedEditMode]->OnMouseRightButtonReleased();
}

bool EditModule::OnMouseDrag()
{
	return _EditModes[_SelectedEditMode]->OnMouseDrag();
}

bool EditModule::OnCopy()
{
	return _EditModes[_SelectedEditMode]->OnCopy();
}

bool EditModule::OnPaste()
{
	return _EditModes[_SelectedEditMode]->OnPaste();
}

bool EditModule::OnMirror()
{
	return _EditModes[_SelectedEditMode]->OnMirror();
}

bool EditModule::OnExpand()
{
	return _EditModes[_SelectedEditMode]->OnExpand();
}

bool EditModule::OnCompress()
{
	return _EditModes[_SelectedEditMode]->OnCompress();
}

bool EditModule::OnReverse()
{
	return _EditModes[_SelectedEditMode]->OnReverse();
}

bool EditModule::OnShuffle()
{
	return _EditModes[_SelectedEditMode]->OnShuffle();
}

bool EditModule::OnQuantize(int InDivisor)
{
	return _EditModes[_SelectedEditMode]->OnQuantize(InDivisor);
}

bool EditModule::OnConvertToHolds(Time Length)
{
    return _EditModes[_SelectedEditMode]->OnConvertToHolds(Length);
}

bool EditModule::OnConvertToTaps()
{
    return _EditModes[_SelectedEditMode]->OnConvertToTaps();
}

bool EditModule::OnMoveAllNotes(Time Offset)
{
    static_Chart->MoveAllNotes(Offset);
    return true;
}

bool EditModule::OnInvertSelection()
{
    // Need to access SelectEditMode or implement selection logic here?
    // EditModule manages _SelectedEditMode.
    // If SelectEditMode is active, call it.
    if (IsEditModeActive<SelectEditMode>())
    {
        // Actually, SelectEditMode does not have OnInvertSelection in base EditMode.
        // We can cast or add it to EditMode interface.
        // Or implement it here using GetSelection?

        // Easier: Just add to EditMode interface (virtual bool OnInvertSelection() { return false; })
        // But for now, let's cast.
        ((SelectEditMode*)_EditModes[_SelectedEditMode])->OnInvertSelection();
        return true;
    }
    return false;
}

bool EditModule::OnUndo()
{
    return static_Chart->Undo();
}

bool EditModule::OnRedo()
{
    return static_Chart->Redo();
}

bool EditModule::OnDelete()
{
	return _EditModes[_SelectedEditMode]->OnDelete();
}

bool EditModule::OnSelectAll()
{
	return _EditModes[_SelectedEditMode]->OnSelectAll();
}

void EditModule::OnEstimateBPM()
{
    if (IsEditModeActive<BpmEditMode>())
    {
        ((BpmEditMode*)_EditModes[_SelectedEditMode])->OnEstimateBPM();
    }
}

void EditModule::OnTap()
{
    if (IsEditModeActive<BpmEditMode>())
    {
        ((BpmEditMode*)_EditModes[_SelectedEditMode])->OnTap();
    }
}

bool EditModule::GetSelectionRange(Time& OutStart, Time& OutEnd)
{
    return _EditModes[_SelectedEditMode]->GetSelectionRange(OutStart, OutEnd);
}

bool EditModule::OnMouseLeftButtonClicked(const bool InIsShiftDown)
{
	return _EditModes[_SelectedEditMode]->OnMouseLeftButtonClicked(InIsShiftDown);
}

void EditModule::SubmitToRenderGraph(TimefieldRenderGraph& InOutTimefieldRenderGraph, const Time InTimeBegin, const Time InTimeEnd)
{
	_EditModes[_SelectedEditMode]->SubmitToRenderGraph(InOutTimefieldRenderGraph, InTimeBegin, InTimeEnd);
}

void EditModule::SetChart(Chart* const InOutChart)
{
	EditMode::SetChart(InOutChart);
}

void EditModule::SetCursorData(const Cursor& InCursor)
{
	EditMode::SetCursorData(InCursor);
}

bool EditModule::Tick(const float& InDeltaTime)
{
	_EditModes[_SelectedEditMode]->Tick();

	return true;
}
