//  Copyright (c) 2016 Hugo Amiard hugo.amiard@laposte.net
//  This software is provided 'as-is' under the zlib License, see the LICENSE.txt file.
//  This notice and the license may not be removed or altered from any source distribution.

#include <toyui/Config.h>
#include <toyui/Widget/Widget.h>

#include <toyui/Widget/RootSheet.h>

#include <toyui/Frame/Layer.h>

#include <toyui/UiLayout.h>
#include <toyui/UiWindow.h>

#include <toyui/Input/InputDevice.h>

namespace toy
{
	std::map<string, Style*> Widget::s_styles;

	Widget::Widget(const Params& params)
		: TypeObject(params.m_type ? *params.m_type : cls<Widget>())
		, m_parent(params.m_parent)
		, m_container(nullptr)
		, m_style(params.m_style)
		, m_frame()
		, m_state(NOSTATE)
		, m_object()
	{
		if(params.m_frameType == MASTER_LAYER || params.m_frameType == LAYER)
			m_frame = make_object<Layer>(as<Wedge>(*this), params.m_frameType);
		else if(params.m_frameType == FRAME)
			m_frame = make_object<Frame>(*this);

		if(m_parent)
		{
			this->updateStyle();
			m_parent->append(*this);
		}
	}

	Widget::Widget(const Params& params, const string& content)
		: Widget(params)
	{
		this->setContent(content);
	}

	Widget::~Widget()
	{}

	RootSheet& Widget::rootSheet()
	{
		return m_parent->rootSheet();
	}

	UiWindow& Widget::uiWindow()
	{
		return this->rootSheet().m_window;
	}

	ControlSwitch& Widget::rootController()
	{
		return this->rootSheet().m_controller;
	}

	void Widget::setContent(const string& content)
	{
		if(content.front() == '(' && content.back() == ')')
		{
			string name(content.begin() + 1, content.end() - 1);
			m_frame->setIcon(&this->uiWindow().findImage(toLower(name)));
		}
		else if(!m_frame->d_icon) // @kludge for buttons whose image is set by the inkstyle
		{
			m_frame->setCaption(content);
		}
	}

	const string& Widget::label()
	{
		return m_frame->d_caption->m_text;
	}

	void Widget::destroySelf()
	{
		this->rootSheet().handleDestroyWidget(*this);
		m_controlGraph = nullptr;
		this->destroyed();
	}

	void Widget::destroyTree()
	{
		this->visit([](Widget& widget, bool&) { widget.destroySelf(); });
	}

	void Widget::destroy()
	{
		this->destroyTree();
		m_container->store().remove(*this);
	}

	void Widget::bind(Wedge& parent, size_t index)
	{
		m_parent = &parent;
		m_index = index;

		this->propagateTo(&parent);
		
		m_frame->bind(m_parent->frame());
	}

	void Widget::unbind()
	{
		m_frame->unbind();
	}

	void Widget::makeSolver()
	{
		m_frame->makeSolver();
	}

	Widget* Widget::findContainer(Type& type)
	{
		Widget* widget = this;

		while(widget)
		{
			if(&widget->m_type == &type)
				return widget;
			widget = widget->m_parent;
		}

		return nullptr;
	}

	void Widget::visit(const Visitor& visitor)
	{
		bool visit;
		visitor(*this, visit);
	}

	void Widget::show()
	{
		m_frame->show();
	}
	
	void Widget::hide()
	{
		m_frame->hide();
	}

	void Widget::updateStyle()
	{
		for(Type* type = &m_type; type && !m_style; type = type->m_base)
			if(s_styles.find(type->m_name) != s_styles.end())
				m_style = s_styles[type->m_name];
		if(!m_style)
			m_style = s_styles["Widget"];
		m_frame->setStyle(*m_style);
	}

	void Widget::setStyle(Style& style, bool hard)
	{
		m_style = &style;
		m_frame->setStyle(*m_style, hard);
	}

	void Widget::toggleState(WidgetState state)
	{
		m_state = static_cast<WidgetState>(m_state ^ state);
		m_frame->updateInkstyle(m_style->skin(m_state));
	}

	void Widget::enableState(WidgetState state)
	{
		if(!(m_state & state))
			this->toggleState(state);
	}

	void Widget::disableState(WidgetState state)
	{
		if(m_state & state)
			this->toggleState(state);
	}

	Widget* Widget::pinpoint(DimFloat pos)
	{
		return this->pinpoint(pos, [](Frame& frame) { return frame.opaque(); });
	}

	Widget* Widget::pinpoint(DimFloat pos, const Frame::Filter& filter)
	{
		Frame* frame = m_frame->pinpoint(pos, filter);
		return frame ? &frame->d_widget : nullptr;
	}

	void Widget::transformEvent(InputEvent& inputEvent)
	{
		if(inputEvent.m_deviceType >= DEVICE_MOUSE)
		{
			MouseEvent& mouseEvent = static_cast<MouseEvent&>(inputEvent);
			mouseEvent.m_relative = m_frame->localPosition(mouseEvent.m_pos);
		}
	}

	InputReceiver* Widget::controlEvent(InputEvent& inputEvent)
	{
		this->transformEvent(inputEvent);

		if(m_controlGraph)
			return m_controlGraph->controlEvent(inputEvent);

		if(inputEvent.m_deviceType >= DEVICE_MOUSE)
		{
			MouseEvent& mouseEvent = static_cast<MouseEvent&>(inputEvent);
			Widget* pinned = this->pinpoint(mouseEvent.m_relative);
			return (pinned && pinned != this) ? pinned->controlEvent(inputEvent) : this;
		}

		return this;
	}

	InputReceiver* Widget::receiveEvent(InputEvent& inputEvent)
	{
		if(inputEvent.m_consumed)
			return this;

		inputEvent.m_visited.push_back(this);

		this->transformEvent(inputEvent);

		return InputAdapter::receiveEvent(inputEvent);
	}

	void Widget::makeActive()
	{
		this->rootSheet().makeActive(*this);
	}

	void Widget::giveControl(InputReceiver& receiver, ControlMode mode, DeviceType device)
	{
		bool control = this->rootController().takeControl(*this, receiver, mode, device);
		if(!control)
			m_controlGraph = make_unique<ControlNode>(receiver, nullptr, mode, device);
	}

	void Widget::takeControl(ControlMode mode, DeviceType device)
	{
		this->rootController().takeControl(*this, mode, device);
	}

	void Widget::yieldControl()
	{
		this->propagateTo(m_parent);
		this->rootController().yieldControl(*this);

		if(m_controlGraph)
			m_controlGraph = nullptr;
	}

	void Widget::control(bool modal)
	{
		this->enableState(CONTROL);
		if(modal)
			this->enableState(MODAL);
	}

	void Widget::uncontrol(bool modal)
	{
		this->disableState(CONTROL);
		if(modal)
			this->disableState(MODAL);
	}

	bool Widget::mouseEntered(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		this->enableState(FOCUSED);
		return false;
	}

	bool Widget::mouseLeaved(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		this->disableState(FOCUSED);
		return false;
	}

	bool Widget::mousePressed(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		this->enableState(TRIGGERED);
		return true;
	}

	bool Widget::mouseReleased(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		this->disableState(TRIGGERED);
		return true;
	}

	void Widget::dirtyLayout()
	{}
}
