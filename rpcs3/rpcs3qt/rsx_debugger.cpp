#include "rsx_debugger.h"
#include "gui_settings.h"
#include "qt_utils.h"
#include "table_item_delegate.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/gcm_printing.h"
#include "util/asm.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QFont>
#include <QFontDatabase>
#include <QPixmap>
#include <QPushButton>
#include <QKeyEvent>

#include <span>

enum GCMEnumTypes
{
	CELL_GCM_ENUM,
	CELL_GCM_PRIMITIVE_ENUM,
};

constexpr auto qstr = QString::fromStdString;

namespace utils
{
	template <typename T, typename U>
	[[nodiscard]] auto bless(const std::span<U>& span)
	{
		return std::span<T>(bless<T>(span.data()), sizeof(U) * span.size() / sizeof(T));
	}
}

rsx_debugger::rsx_debugger(std::shared_ptr<gui_settings> gui_settings, QWidget* parent)
	: QDialog(parent)
	, m_gui_settings(std::move(gui_settings))
{
	setWindowTitle(tr("RSX Debugger"));
	setObjectName("rsx_debugger");
	setWindowFlags(Qt::Window);

	// Fonts and Colors
	QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	mono.setPointSize(8);
	QLabel l("000000000"); // hacky way to get the lineedit to resize properly
	l.setFont(mono);

	QHBoxLayout* hbox_controls_addr = new QHBoxLayout();

	// Controls: Breaks
	QPushButton* b_break_frame = new QPushButton(tr("Frame"));
	QPushButton* b_break_text  = new QPushButton(tr("Texture"));
	QPushButton* b_break_draw  = new QPushButton(tr("Draw"));
	QPushButton* b_break_prim  = new QPushButton(tr("Primitive"));
	QPushButton* b_break_inst  = new QPushButton(tr("Command"));
	b_break_frame->setAutoDefault(false);
	b_break_text->setAutoDefault(false);
	b_break_draw->setAutoDefault(false);
	b_break_prim->setAutoDefault(false);
	b_break_inst->setAutoDefault(false);

	QHBoxLayout* hbox_controls_breaks = new QHBoxLayout();
	hbox_controls_breaks->addWidget(b_break_frame);
	hbox_controls_breaks->addWidget(b_break_text);
	hbox_controls_breaks->addWidget(b_break_draw);
	hbox_controls_breaks->addWidget(b_break_prim);
	hbox_controls_breaks->addWidget(b_break_inst);

	QGroupBox* gb_controls_breaks = new QGroupBox(tr("Break on:"));
	gb_controls_breaks->setLayout(hbox_controls_breaks);

	// TODO: This feature is not yet implemented
	b_break_frame->setEnabled(false);
	b_break_text->setEnabled(false);
	b_break_draw->setEnabled(false);
	b_break_prim->setEnabled(false);
	b_break_inst->setEnabled(false);

	QHBoxLayout* hbox_controls = new QHBoxLayout();
	hbox_controls->addWidget(gb_controls_breaks);
	hbox_controls->addStretch(1);

	m_tw_rsx = new QTabWidget();

	// adds a tab containing a list to the tabwidget
	const auto add_rsx_tab = [this, &mono](const QString& tabname, int columns)
	{
		QTableWidget* table = new QTableWidget();
		table->setItemDelegate(new table_item_delegate);
		table->setFont(mono);
		table->setGridStyle(Qt::NoPen);
		table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
		table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		table->setSelectionBehavior(QAbstractItemView::SelectRows);
		table->verticalHeader()->setVisible(false);
		table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
		table->verticalHeader()->setDefaultSectionSize(16);
		table->horizontalHeader()->setStretchLastSection(true);
		table->setColumnCount(columns);
		m_tw_rsx->addTab(table, tabname);
		return table;
	};

	m_list_captured_frame      = add_rsx_tab(tr("Captured Frame"), 1);
	m_list_captured_draw_calls = add_rsx_tab(tr("Captured Draw Calls"), 1);

	m_list_captured_frame->setHorizontalHeaderLabels(QStringList() << tr("Column"));
	m_list_captured_frame->setColumnWidth(0, 720);

	m_list_captured_draw_calls->setHorizontalHeaderLabels(QStringList() << tr("Draw calls"));
	m_list_captured_draw_calls->setColumnWidth(0, 720);

	// Tools: Tools = Controls + Notebook Tabs
	QVBoxLayout* vbox_tools = new QVBoxLayout();
	vbox_tools->addLayout(hbox_controls);
	vbox_tools->addWidget(m_tw_rsx);

	// State explorer
	m_text_transform_program = new QLabel();
	m_text_transform_program->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_text_transform_program->setFont(mono);
	m_text_transform_program->setText("");

	m_text_shader_program = new QLabel();
	m_text_shader_program->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_text_shader_program->setFont(mono);
	m_text_shader_program->setText("");

	m_list_index_buffer = new QListWidget();
	m_list_index_buffer->setFont(mono);

	// Panels for displaying the buffers
	m_buffer_colorA  = new Buffer(false, 0, tr("Color Buffer A"), this);
	m_buffer_colorB  = new Buffer(false, 1, tr("Color Buffer B"), this);
	m_buffer_colorC  = new Buffer(false, 2, tr("Color Buffer C"), this);
	m_buffer_colorD  = new Buffer(false, 3, tr("Color Buffer D"), this);
	m_buffer_depth   = new Buffer(false, 4, tr("Depth Buffer"), this);
	m_buffer_stencil = new Buffer(false, 4, tr("Stencil Buffer"), this);
	m_buffer_tex     = new Buffer(true, 4, tr("Texture"), this);

	// Merge and display everything
	QVBoxLayout* vbox_buffers1 = new QVBoxLayout();
	vbox_buffers1->addWidget(m_buffer_colorA);
	vbox_buffers1->addWidget(m_buffer_colorC);
	vbox_buffers1->addWidget(m_buffer_depth);
	vbox_buffers1->addWidget(m_buffer_tex);
	vbox_buffers1->addStretch();

	QVBoxLayout* vbox_buffers2 = new QVBoxLayout();
	vbox_buffers2->addWidget(m_buffer_colorB);
	vbox_buffers2->addWidget(m_buffer_colorD);
	vbox_buffers2->addWidget(m_buffer_stencil);
	vbox_buffers2->addStretch();

	QHBoxLayout* buffer_layout = new QHBoxLayout();
	buffer_layout->addLayout(vbox_buffers1);
	buffer_layout->addLayout(vbox_buffers2);
	buffer_layout->addStretch();

	QWidget* buffers = new QWidget();
	buffers->setLayout(buffer_layout);

	QTabWidget* state_rsx = new QTabWidget();
	state_rsx->addTab(buffers, tr("RTTs and DS"));
	state_rsx->addTab(m_text_transform_program, tr("Transform program"));
	state_rsx->addTab(m_text_shader_program, tr("Shader program"));
	state_rsx->addTab(m_list_index_buffer, tr("Index buffer"));

	QHBoxLayout* main_layout = new QHBoxLayout();
	main_layout->addLayout(vbox_tools, 1);
	main_layout->addWidget(state_rsx, 1);
	setLayout(main_layout);

	connect(m_list_captured_draw_calls, &QTableWidget::itemClicked, this, &rsx_debugger::OnClickDrawCalls);

	// Restore header states
	QVariantMap states = m_gui_settings->GetValue(gui::rsx_states).toMap();
	for (int i = 0; i < m_tw_rsx->count(); i++)
		(static_cast<QTableWidget*>(m_tw_rsx->widget(i)))->horizontalHeader()->restoreState(states[QString::number(i)].toByteArray());

	// Fill the frame
	for (u32 i = 0; i < frame_debug.command_queue.size(); i++)
		m_list_captured_frame->insertRow(i);

	if (!restoreGeometry(m_gui_settings->GetValue(gui::rsx_geometry).toByteArray()))
		UpdateInformation();
}

void rsx_debugger::closeEvent(QCloseEvent* event)
{
	// Save header states and window geometry
	QVariantMap states;
	for (int i = 0; i < m_tw_rsx->count(); i++)
		states[QString::number(i)] = (static_cast<QTableWidget*>(m_tw_rsx->widget(i)))->horizontalHeader()->saveState();

	m_gui_settings->SetValue(gui::rsx_states, states);
	m_gui_settings->SetValue(gui::rsx_geometry, saveGeometry());

	QDialog::closeEvent(event);
}

void rsx_debugger::keyPressEvent(QKeyEvent* event)
{
	if (isActiveWindow() && !event->isAutoRepeat())
	{
		switch (event->key())
		{
		case Qt::Key_F5: UpdateInformation(); break;
		default: break;
		}
	}

	QDialog::keyPressEvent(event);
}

bool rsx_debugger::eventFilter(QObject* object, QEvent* event)
{
	if (Buffer* buffer = qobject_cast<Buffer*>(object))
	{
		switch (event->type())
		{
		case QEvent::MouseButtonDblClick:
		{
			buffer->ShowWindowed();
			break;
		}
		default:
			break;
		}
	}

	return QDialog::eventFilter(object, event);
}

Buffer::Buffer(bool isTex, u32 id, const QString& name, QWidget* parent)
	: QGroupBox(name, parent)
	, m_id(id)
	, m_isTex(isTex)
{
	m_image_size = isTex ? Texture_Size : Panel_Size;

	m_canvas = new QLabel();
	m_canvas->setFixedSize(m_image_size);

	QHBoxLayout* layout = new QHBoxLayout();
	layout->setContentsMargins(1, 1, 1, 1);
	layout->addWidget(m_canvas);
	setLayout(layout);

	installEventFilter(parent);
}

// Draws a formatted and buffered <image> inside the Buffer Widget
void Buffer::showImage(const QImage& image)
{
	if (image.isNull())
		return;

	m_image = image;
	const QImage scaled = m_image.scaled(m_image_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_canvas->setPixmap(QPixmap::fromImage(scaled));

	QHBoxLayout* new_layout = new QHBoxLayout();
	new_layout->setContentsMargins(1, 1, 1, 1);
	new_layout->addWidget(m_canvas);
	delete layout();
	setLayout(new_layout);
}

void Buffer::ShowWindowed() const
{
	//const auto render = rsx::get_current_renderer();
	if (!g_fxo->is_init<rsx::thread>())
		return;

	// TODO: Is there any better way to choose the color buffers
	//if (0 <= m_id && m_id < 4)
	//{
	//	const auto buffers = render->display_buffers;
	//	u32 addr = rsx::constants::local_mem_base + buffers[m_id].offset;
	//	if (vm::check_addr(addr) && buffers[m_id].width && buffers[m_id].height)
	//		memory_viewer_panel::ShowImage(this, addr, 3, buffers[m_id].width, buffers[m_id].height, true);
	//	return;
	//}

	gui::utils::show_windowed_image(m_image, title());

	//if (m_isTex)
	//{
	//	u8 location = render->textures[m_cur_texture].location();
	//	if (location <= 1 && vm::check_addr(rsx::get_address(render->textures[m_cur_texture].offset(), location))
	//		&& render->textures[m_cur_texture].width() && render->textures[m_cur_texture].height())
	//		memory_viewer_panel::ShowImage(this,
	//			rsx::get_address(render->textures[m_cur_texture].offset(), location), 1,
	//			render->textures[m_cur_texture].width(),
	//			render->textures[m_cur_texture].height(), false);
	//}
}

namespace
{
	f32 f16_to_f32(f16 val)
	{
		// See http://stackoverflow.com/a/26779139
		// The conversion doesn't handle NaN/Inf

		const u16 _u16 = static_cast<u16>(val);
		const u32 raw = ((_u16 & 0x8000) << 16) |             // Sign (just moved)
		                (((_u16 & 0x7c00) + 0x1C000) << 13) | // Exponent ( exp - 15 + 127)
		                ((_u16 & 0x03FF) << 13);              // Mantissa

		return std::bit_cast<f32>(raw);
	}

	std::array<u8, 3> get_value(std::span<const std::byte> orig_buffer, rsx::surface_color_format format, usz idx)
	{
		switch (format)
		{
		case rsx::surface_color_format::b8:
		{
			const u8 value = utils::bless<const u8>(orig_buffer)[idx];
			return{ value, value, value };
		}
		case rsx::surface_color_format::x32:
		{
			const be_t<u32> stored_val = utils::bless<const be_t<u32>>(orig_buffer)[idx];
			const u32 swapped_val = stored_val;
			const f32 float_val = std::bit_cast<f32>(swapped_val);
			const u8 val = float_val * 255.f;
			return{ val, val, val };
		}
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		{
			const auto ptr = utils::bless<const u8>(orig_buffer);
			return{ ptr[1 + idx * 4], ptr[2 + idx * 4], ptr[3 + idx * 4] };
		}
		case rsx::surface_color_format::a8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		{
			const auto ptr = utils::bless<const u8>(orig_buffer);
			return{ ptr[3 + idx * 4], ptr[2 + idx * 4], ptr[1 + idx * 4] };
		}
		case rsx::surface_color_format::w16z16y16x16:
		{
			const auto ptr = utils::bless<const u16>(orig_buffer);
			const f16 h0 = static_cast<f16>(ptr[4 * idx]);
			const f16 h1 = static_cast<f16>(ptr[4 * idx + 1]);
			const f16 h2 = static_cast<f16>(ptr[4 * idx + 2]);
			const f32 f0 = f16_to_f32(h0);
			const f32 f1 = f16_to_f32(h1);
			const f32 f2 = f16_to_f32(h2);

			const u8 val0 = f0 * 255.f;
			const u8 val1 = f1 * 255.f;
			const u8 val2 = f2 * 255.f;
			return{ val0, val1, val2 };
		}
		case rsx::surface_color_format::g8b8:
		case rsx::surface_color_format::r5g6b5:
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		case rsx::surface_color_format::w32z32y32x32:
		default:
			fmt::throw_exception("Unsupported format for display");
		}
	}

	/**
	 * Return a new buffer that can be passed to QImage.
	 */
	u8* convert_to_QImage_buffer(rsx::surface_color_format format, std::span<const std::byte> orig_buffer, usz width, usz height) noexcept
	{
		u8* buffer = static_cast<u8*>(std::malloc(width * height * 4));
		if (!buffer || width == 0 || height == 0)
		{
			return nullptr;
		}
		for (u32 i = 0; i < width * height; i++)
		{
			// depending on original buffer, the colors may need to be reversed
			const auto &colors = get_value(orig_buffer, format, i);
			buffer[0 + i * 4] = colors[0];
			buffer[1 + i * 4] = colors[1];
			buffer[2 + i * 4] = colors[2];
			buffer[3 + i * 4] = 255;
		}
		return buffer;
	}
}

void rsx_debugger::OnClickDrawCalls()
{
	const usz draw_id = m_list_captured_draw_calls->currentRow();

	const auto& draw_call = frame_debug.draw_calls[draw_id];

	Buffer* buffers[] =
	{
		m_buffer_colorA,
		m_buffer_colorB,
		m_buffer_colorC,
		m_buffer_colorD,
	};

	const u32 width = draw_call.state.surface_clip_width();
	const u32 height = draw_call.state.surface_clip_height();

	for (usz i = 0; i < 4; i++)
	{
		if (width && height && !draw_call.color_buffer[i].empty())
		{
			unsigned char* buffer = convert_to_QImage_buffer(draw_call.state.surface_color(), draw_call.color_buffer[i], width, height);
			buffers[i]->showImage(QImage(buffer, static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32, [](void* buffer){ std::free(buffer); }, buffer));
		}
	}

	// Buffer Z
	{
		if (width && height && !draw_call.depth_stencil[0].empty())
		{
			const std::span<const std::byte> orig_buffer = draw_call.depth_stencil[0];
			u8* buffer = static_cast<u8*>(std::malloc(4ULL * width * height));

			if (draw_call.state.surface_depth_fmt() == rsx::surface_depth_format::z24s8)
			{
				for (u32 row = 0; row < height; row++)
				{
					for (u32 col = 0; col < width; col++)
					{
						const u32 depth_val = utils::bless<const u32>(orig_buffer)[row * width + col];
						const u8 displayed_depth_val = 255 * depth_val / 0xFFFFFF;
						buffer[4 * col + 0 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 1 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 2 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 3 + width * row * 4] = 255;
					}
				}
			}
			else
			{
				for (u32 row = 0; row < height; row++)
				{
					for (u32 col = 0; col < width; col++)
					{
						const u16 depth_val = utils::bless<const u16>(orig_buffer)[row * width + col];
						const u8 displayed_depth_val = 255 * depth_val / 0xFFFF;
						buffer[4 * col + 0 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 1 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 2 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 3 + width * row * 4] = 255;
					}
				}
			}
			m_buffer_depth->showImage(QImage(buffer, static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32, [](void* buffer){ std::free(buffer); }, buffer));
		}
	}

	// Buffer S
	{
		if (width && height && !draw_call.depth_stencil[1].empty())
		{
			const std::span<const std::byte> orig_buffer = draw_call.depth_stencil[1];
			u8* buffer = static_cast<u8*>(std::malloc(4ULL * width * height));

			for (u32 row = 0; row < height; row++)
			{
				for (u32 col = 0; col < width; col++)
				{
					const u8 stencil_val = utils::bless<const u8>(orig_buffer)[row * width + col];
					buffer[4 * col + 0 + width * row * 4] = stencil_val;
					buffer[4 * col + 1 + width * row * 4] = stencil_val;
					buffer[4 * col + 2 + width * row * 4] = stencil_val;
					buffer[4 * col + 3 + width * row * 4] = 255;
				}
			}
			m_buffer_stencil->showImage(QImage(buffer, static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32));
		}
	}

	// Programs
	m_text_transform_program->clear();
	m_text_transform_program->setText(qstr(frame_debug.draw_calls[draw_id].programs.first));
	m_text_shader_program->clear();
	m_text_shader_program->setText(qstr(frame_debug.draw_calls[draw_id].programs.second));

	m_list_index_buffer->clear();
	//m_list_index_buffer->insertColumn(0, "Index", 0, 700);
	if (frame_debug.draw_calls[draw_id].state.index_type() == rsx::index_array_type::u16)
	{
		u16 *index_buffer = reinterpret_cast<u16*>(frame_debug.draw_calls[draw_id].index.data());
		for (u32 i = 0; i < frame_debug.draw_calls[draw_id].vertex_count; ++i)
		{
			m_list_index_buffer->insertItem(i, qstr(std::to_string(index_buffer[i])));
		}
	}
	if (frame_debug.draw_calls[draw_id].state.index_type() == rsx::index_array_type::u32)
	{
		u32 *index_buffer = reinterpret_cast<u32*>(frame_debug.draw_calls[draw_id].index.data());
		for (u32 i = 0; i < frame_debug.draw_calls[draw_id].vertex_count; ++i)
		{
			m_list_index_buffer->insertItem(i, qstr(std::to_string(index_buffer[i])));
		}
	}
}

void rsx_debugger::UpdateInformation() const
{
	GetMemory();
	GetBuffers();
}

void rsx_debugger::GetMemory() const
{
	std::string dump;
	u32 cmd_i = 0;

	for (const auto& command : frame_debug.command_queue)
	{
		const std::string str = rsx::get_pretty_printing_function(command.first)(command.first, command.second);
		m_list_captured_frame->setItem(cmd_i++, 0, new QTableWidgetItem(qstr(str)));

		dump += str;
		dump += '\n';
	}

	if (fs::file file = fs::file(fs::get_cache_dir() + "command_dump.log", fs::rewrite))
	{
		file.write(dump);
	}

	for (u32 i = 0;i < frame_debug.draw_calls.size(); i++)
		m_list_captured_draw_calls->setItem(i, 0, new QTableWidgetItem(qstr(frame_debug.draw_calls[i].name)));
}

void rsx_debugger::GetBuffers() const
{
	const auto render = rsx::get_current_renderer();
	if (!render)
	{
		return;
	}

	// Draw Buffers
	// TODO: Currently it only supports color buffers
	for (u32 bufferId=0; bufferId < render->display_buffers_count; bufferId++)
	{
		const auto buffers = render->display_buffers;
		const u32 rsx_buffer_addr = rsx::constants::local_mem_base + buffers[bufferId].offset;

		const u32 width  = buffers[bufferId].width;
		const u32 height = buffers[bufferId].height;

		if (!vm::check_addr(rsx_buffer_addr, vm::page_readable, width * height * 4))
			continue;

		const auto rsx_buffer = vm::get_super_ptr<const u8>(rsx_buffer_addr);

		u8* buffer = static_cast<u8*>(std::malloc(4ULL * width * height));

		// ABGR to ARGB and flip vertically
		for (u32 y = 0; y < height; y++)
		{
			for (u32 i = 0, j = 0; j < width * 4; i += 4, j += 4)
			{
				buffer[i + 0 + y * width * 4] = rsx_buffer[j + 1 + (height - y - 1) * width * 4]; // B
				buffer[i + 1 + y * width * 4] = rsx_buffer[j + 2 + (height - y - 1) * width * 4]; // G
				buffer[i + 2 + y * width * 4] = rsx_buffer[j + 3 + (height - y - 1) * width * 4]; // R
				buffer[i + 3 + y * width * 4] = rsx_buffer[j + 0 + (height - y - 1) * width * 4]; // A
			}
		}

		// TODO: Is there any better way to clasify the color buffers? How can we include the depth and stencil buffers?
		Buffer* pnl;
		switch(bufferId)
		{
		case 0:  pnl = m_buffer_colorA; break;
		case 1:  pnl = m_buffer_colorB; break;
		case 2:  pnl = m_buffer_colorC; break;
		default: pnl = m_buffer_colorD; break;
		}
		pnl->showImage(QImage(buffer, width, height, QImage::Format_RGB32, [](void* buffer){ std::free(buffer); }, buffer));
	}

	// Draw Texture
	//if (!render->textures[m_cur_texture].enabled())
	//	return;

	//u32 offset = render->textures[m_cur_texture].offset();

	//if(!offset)
	//	return;

	//u8 location = render->textures[m_cur_texture].location();

	//if(location > 1)
	//	return;

	//u32 TexBuffer_addr = rsx::get_address(offset, location);

	//if(!vm::check_addr(TexBuffer_addr))
	//	return;

	//unsigned char* TexBuffer = vm::get_super_ptr<u8>(TexBuffer_addr);

	//const u32 width  = render->textures[m_cur_texture].width();
	//const u32 height = render->textures[m_cur_texture].height();
	//unsigned char* buffer = (unsigned char*)malloc(width * height * 3);
	//std::memcpy(buffer, vm::base(TexBuffer_addr), width * height * 3);

	//m_buffer_tex->showImage(QImage(buffer, m_text_width, m_text_height, QImage::Format_RGB32));
}
