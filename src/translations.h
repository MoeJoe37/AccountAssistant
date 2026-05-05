#pragma once
#include <QString>
#include <QStringList>
#include <QLocale>
#include <Qt>

enum class AppLanguage { English, Arabic };
inline AppLanguage g_lang      = AppLanguage::English;
inline bool        g_lightMode = false;

enum class AppCurrency { USD, IQD };
inline AppCurrency g_currency    = AppCurrency::USD;
inline int         g_fontSize    = 12;  // 12, 14, or 16
inline bool        g_classicView = false; // classic spreadsheet table vs. card view

inline bool isArabic()  { return g_lang == AppLanguage::Arabic; }
inline bool isEnglish() { return g_lang == AppLanguage::English; }
inline Qt::LayoutDirection appLayoutDirection()
{
    return isArabic() ? Qt::RightToLeft : Qt::LeftToRight;
}
inline Qt::Alignment appTextAlign()
{
    return isArabic() ? Qt::AlignRight : Qt::AlignLeft;
}

inline QString currencyPrefix()
{
    if (g_currency == AppCurrency::USD)
        return QStringLiteral("$ ");
    return isArabic() ? QString::fromUtf8("دع ") : QString();
}

inline QString currencySuffix()
{
    if (g_currency == AppCurrency::IQD && isEnglish())
        return QStringLiteral(" IQD");
    return QString();
}

inline int currencyDecimals()
{
    return g_currency == AppCurrency::IQD ? 0 : 2;
}

inline QString formatCurrencyNumber(double value, int decimalsOverride = -1)
{
    const int decimals = decimalsOverride >= 0 ? decimalsOverride : currencyDecimals();
    return QLocale(QLocale::English, QLocale::UnitedStates).toString(value, 'f', decimals);
}

inline QString formatMoneyText(double value, int decimalsOverride = -1)
{
    return currencyPrefix() + formatCurrencyNumber(value, decimalsOverride) + currencySuffix();
}


inline QString T(const char* en, const char* ar)
{
    return g_lang == AppLanguage::English
        ? QString::fromUtf8(en) : QString::fromUtf8(ar);
}

inline QString tr_expense_months_dropdown_label_62ac11() { return T("Months", "الأشهر"); }
inline QString tr_expense_amount_field_93a771() { return T("Amount", "المبلغ"); }
inline QString tr_expense_account_type_field_a870c9() { return T("Account type", "نوع الحساب"); }
inline QString tr_fixed_expenses_subtitle_a65f2a() { return T("Select a month, then enter each fixed account amount and choose whether it is account receivable or account payable.", "اختر الشهر ثم أدخل مبلغ كل حساب ثابت وحدد هل هو حساب مدين أم حساب دائن."); }
inline QString tr_fixed_expense_account_header_a13bcd() { return T("Account", "الحساب"); }
inline QString tr_group_by_2bda9d() { return T("Group by", "تجميع حسب"); }
inline QString tr_add_account_9d4f6c() { return T("+  Add account", "+  إضافة حساب"); }
inline QString tr_delete_account_6dd013() { return T("Delete account", "حذف الحساب"); }
inline QString tr_new_expense_account_title_d82b71() { return T("Add account", "إضافة حساب"); }
inline QString tr_new_expense_account_prompt_9747ab() { return T("Account name:", "اسم الحساب:"); }
inline QString tr_delete_account_title_b23407() { return T("Delete account", "حذف الحساب"); }
inline QString tr_delete_account_warning_f0c88a() { return T("Delete this account from all months?", "هل تريد حذف هذا الحساب من جميع الأشهر؟"); }
inline QString tr_fixed_expense_salaries_wages_2f4d01() { return T("Salaries and wages", "رواتب واجور"); }
inline QString tr_fixed_expense_monthly_incentives_d9f522() { return T("Monthly incentives", "الحوافز الشهرية"); }
inline QString tr_fixed_expense_rewards_allowances_a137bc() { return T("Bonuses and allowances", "مكافات واكراميات"); }
inline QString tr_fixed_expense_fuel_oil_893654() { return T("Fuel and oil", "وقود وزيوت"); }
inline QString tr_fixed_expense_supplies_consumables_5a1e7b() { return T("Supplies and consumables", "لوازم ومهمات"); }
inline QString tr_fixed_expense_stationery_4cb5e7() { return T("Stationery", "قرطاسية"); }
inline QString tr_fixed_expense_building_maintenance_f4d9e0() { return T("Building maintenance", "صيانة مباني"); }
inline QString tr_fixed_expense_furniture_maintenance_bbd26c() { return T("Furniture maintenance", "صيانة الاثاث"); }
inline QString tr_fixed_expense_vehicle_transport_maintenance_55de12() { return T("Vehicle maintenance and transportation", "صيانة وسائط نقل وانتقال"); }
inline QString tr_fixed_expense_advertising_publicity_c7c307() { return T("Advertising and publicity", "دعاية واعلان"); }
inline QString tr_fixed_expense_transport_dispatch_comms_9fc099() { return T("Transport, dispatch, and communications", "نقل وايفاد واتصالات"); }
inline QString tr_fixed_expense_transport_vehicle_rental_eab8a0() { return T("Transportation vehicle rental", "استئجار وسائط نقل وانتقال"); }
inline QString tr_fixed_expense_building_rent_8d5f9d() { return T("Building rent", "استئجار مباني"); }
inline QString tr_fixed_expense_other_services_3a9a8b() { return T("Other service expenses", "مصروفات خدمية اخرى"); }
inline QString tr_fixed_expense_bank_expenses_eb4a39() { return T("Bank expenses", "مصاريف بنك"); }
inline QString tr_fixed_expense_gifts_donations_045a77() { return T("Gifts and donations", "هدايا وتبرعات"); }
inline QString tr_fixed_expense_taxes_misc_fees_292d30() { return T("Taxes and miscellaneous fees", "ضرائب ورسوم متنوعة"); }
inline QString tr_fixed_expense_distribution_expenses_5532ab() { return T("Distribution expenses", "مصاريف التوزيع"); }
inline QString tr_fixed_expense_promotional_allowance_244fc1() { return T("Promotional allowance", "سماح تشجيعي"); }
inline QString tr_fixed_expense_cash_allowance_559de3() { return T("Cash allowance", "سماح نقدي"); }
inline QString tr_fixed_expense_price_difference_allowance_98a361() { return T("Price difference allowance", "سماح فرق السعر"); }
inline QString tr_fixed_expense_damaged_goods_allowance_45fe78() { return T("Damaged goods allowance", "سماح تالف"); }
inline QString tr_fixed_expense_gift_allowance_892dd1() { return T("Gift allowance", "سماح هدايا"); }
inline QString tr_fixed_expense_inventory_damage_f880ca() { return T("Damaged inventory", "تالف المخزون السلعي"); }

inline QStringList fixedExpenseAccountNames()
{
    return {
        tr_fixed_expense_salaries_wages_2f4d01(),
        tr_fixed_expense_monthly_incentives_d9f522(),
        tr_fixed_expense_rewards_allowances_a137bc(),
        tr_fixed_expense_fuel_oil_893654(),
        tr_fixed_expense_supplies_consumables_5a1e7b(),
        tr_fixed_expense_stationery_4cb5e7(),
        tr_fixed_expense_building_maintenance_f4d9e0(),
        tr_fixed_expense_furniture_maintenance_bbd26c(),
        tr_fixed_expense_vehicle_transport_maintenance_55de12(),
        tr_fixed_expense_advertising_publicity_c7c307(),
        tr_fixed_expense_transport_dispatch_comms_9fc099(),
        tr_fixed_expense_transport_vehicle_rental_eab8a0(),
        tr_fixed_expense_building_rent_8d5f9d(),
        tr_fixed_expense_other_services_3a9a8b(),
        tr_fixed_expense_bank_expenses_eb4a39(),
        tr_fixed_expense_gifts_donations_045a77(),
        tr_fixed_expense_taxes_misc_fees_292d30(),
        tr_fixed_expense_distribution_expenses_5532ab(),
        tr_fixed_expense_promotional_allowance_244fc1(),
        tr_fixed_expense_cash_allowance_559de3(),
        tr_fixed_expense_price_difference_allowance_98a361(),
        tr_fixed_expense_damaged_goods_allowance_45fe78(),
        tr_fixed_expense_gift_allowance_892dd1(),
        tr_fixed_expense_inventory_damage_f880ca()
    };
}

inline int fixedExpenseAccountCount()
{
    return fixedExpenseAccountNames().size();
}

inline QString fixedExpenseAccountCode(int index)
{
    return QStringLiteral("FX%1").arg(index + 1, 2, 10, QChar('0'));
}


inline QString tr_account_assistant_edcbbf() { return T("Account Assistant", "\u0645\u0633\u0627\u0639\u062F \u0627\u0644\u062D\u0633\u0627\u0628\u0627\u062A"); }
inline QString tr_account_assistant_financial_re_7851db() { return T("Accounting Report", "تقرير محاسبي"); }
inline QString tr_account_payable_003206() { return T("Account Payable", "حساب دائن"); }
inline QString tr_account_receivable_59bf34() { return T("Account Receivable", "حساب مدين"); }
inline QString tr_accounts_08f9e5() { return T("Accounts", "الحسابات"); }
inline QString tr_accounts_9c0541() { return T("  ▣  Accounts  ", "  ▣  الحسابات  "); }
inline QString tr_add_a98dbf() { return T("+  Add", "+  إضافة"); }
inline QString tr_add_comparison_1c963e() { return T("+  Add Comparison", "+  إضافة مقارنة"); }
inline QString tr_add_expense_accounts_here_thes_d000bc() { return T("Add expense accounts here. These accounts are independent from the monthly table.", "أضف حسابات المصروفات هنا. هذه الحسابات مستقلة عن جدول الأشهر."); }
inline QString tr_add_page_separator_below_862284() { return T("Add page separator below", "إضافة فاصل صفحة أسفلها"); }
inline QString tr_all_b4d286() { return T("All", "الكل"); }
inline QString tr_all_months_428b74() { return T("All months", "كل الأشهر"); }
inline QString tr_all_months_e73b82() { return T("ALL MONTHS", "كل الأشهر"); }
inline QString tr_all_months_summary_b46139() { return T("All months (summary)", "كل الأشهر (ملخص)"); }
inline QString tr_an_account_with_this_name_alre_7f9103() { return T("An account with this name already exists.", "يوجد حساب بهذا الاسم بالفعل."); }
inline QString tr_appearance_97f3e4() { return T("Appearance", "\u0627\u0644\u0645\u0638\u0647\u0631"); }
inline QString tr_apply_042b05() { return T("Apply", "\u062a\u0637\u0628\u064a\u0642"); }
inline QString tr_ascending_c0fe46() { return T("Ascending", "تصاعدي"); }
inline QString tr_average_7302d6() { return T("Average", "\u0627\u0644\u0645\u062A\u0648\u0633\u0637"); }
inline QString tr_avg_gap_7363da() { return T("Avg gap", "\u0645\u062A\u0648\u0633\u0637 \u0627\u0644\u0641\u0627\u0631\u0642"); }
inline QString tr_bar_6dda02() { return T("Bar", "أعمدة"); }
inline QString tr_bar_chart_a5f324() { return T("Bar chart", "مخطط أعمدة"); }
inline QString tr_calculate_36f437() { return T("▶  Calculate", "\u25B6  \u0627\u062D\u0633\u0628"); }
inline QString tr_cancel_8d40ef() { return T("Cancel", "إلغاء"); }
inline QString tr_cancel_b879b2() { return T("Cancel", "\u0625\u0644\u063a\u0627\u0621"); }
inline QString tr_candle_77e8b9() { return T("Candle", "شمعة"); }
inline QString tr_candle_chart_f7a9c2() { return T("Candle chart", "مخطط شموع"); }
inline QString tr_chart_block_583f01() { return T("Chart block", "كتلة الرسم"); }
inline QString tr_chart_preview_9abc22() { return T("Chart preview", "\u0645\u0639\u0627\u064A\u0646\u0629 \u0627\u0644\u0631\u0633\u0645"); }
inline QString tr_chart_type_bd42b2() { return T("Chart type", "نوع الرسم"); }
inline QString tr_charts_ced4c1() { return T("Charts", "الرسوم"); }
inline QString tr_choose_months_ff1808() { return T("Choose months", "اختيار الأشهر"); }
inline QString tr_choose_one_or_more_month_cards_18cee3() { return T("Choose one or more month cards, then place them before or after the charts.", "اختر بطاقة شهر واحدة أو أكثر ثم ضعها قبل المخططات أو بعدها."); }
inline QString tr_choose_what_appears_in_results_05286a() { return T("Choose what appears in Results", "اختر ما يظهر في النتائج"); }
inline QString tr_classic_table_view_spreadsheet_b69d40() { return T("Classic Table View (spreadsheet layout)", "\u0639\u0631\u0636 \u0627\u0644\u062c\u062f\u0648\u0644 \u0627\u0644\u0643\u0644\u0627\u0633\u064a\u0643\u064a"); }
inline QString tr_clear_all_data_491f5d() { return T("Clear All Data", "مسح جميع البيانات"); }
inline QString tr_clear_data_4fcd0d() { return T("🗑  Clear Data", "🗑  مسح البيانات"); }
inline QString tr_auto_clear_data_entry_7e6a91f0() { return T("Clear Data Entry", "مسح إدخال البيانات"); }
inline QString tr_auto_clear_expenses_6c70d1bb() { return T("Clear Expenses", "مسح المصروفات"); }
inline QString tr_auto_clear_suppliers_fce9cf44() { return T("Clear Suppliers", "مسح الموردين"); }
inline QString tr_auto_clear_data_entry_warning_29d42ac4() { return T("This will erase only the Data Entry tab values. Expenses and Suppliers will not be changed.\n\nAre you sure you want to continue?", "سيتم حذف قيم تبويب إدخال البيانات فقط. لن يتم تغيير المصروفات أو الموردين.\n\nهل أنت متأكد من المتابعة؟"); }
inline QString tr_auto_clear_expenses_warning_7be9d308() { return T("This will erase only the Expenses tab accounts. Data Entry and Suppliers will not be changed.\n\nAre you sure you want to continue?", "سيتم حذف حسابات تبويب المصروفات فقط. لن يتم تغيير إدخال البيانات أو الموردين.\n\nهل أنت متأكد من المتابعة؟"); }
inline QString tr_auto_clear_suppliers_warning_0c6309c6() { return T("This will erase only the Suppliers tab data. Data Entry and Expenses will not be changed.\n\nAre you sure you want to continue?", "سيتم حذف بيانات تبويب الموردين فقط. لن يتم تغيير إدخال البيانات أو المصروفات.\n\nهل أنت متأكد من المتابعة؟"); }
inline QString tr_closing_stock_last_period_a0c5b2() { return T("Closing Stock (Last Period)", "اخر المدة"); }
inline QString tr_cogs_d716f1() { return T("COGS", "تكلفة البضاعة"); }
inline QString tr_cogs_vs_profit_margin_fd48e9() { return T("COGS vs Trading Result", "تكلفة مقابل نتيجة التداول"); }
inline QString tr_comparison_title_optional_fae13e() { return T("Comparison title (optional)", "عنوان اختياري"); }
inline QString tr_cost_of_goods_sold_31b73d() { return T("Cost of Goods Sold", "\u062a\u0643\u0644\u0641\u0629 \u0627\u0644\u0628\u0636\u0627\u0639\u0629"); }
inline QString tr_cost_of_goods_sold_55196f() { return T("Cost of Goods Sold", "\u062A\u0643\u0644\u0641\u0629 \u0627\u0644\u0628\u0636\u0627\u0639\u0629"); }
inline QString tr_cost_of_goods_sold_6e7684() { return T("Cost of Goods Sold", "تكلفة البضاعة"); }
inline QString tr_currency_88f072() { return T("Currency", "\u0627\u0644\u0639\u0645\u0644\u0629"); }
inline QString tr_custom_comparisons_63300f() { return T("Custom comparisons", "المقارنات المخصصة"); }
inline QString tr_more_metrics_000000() { return T("More metrics", "المزيد من المقاييس"); }
inline QString tr_count_as_100_percent_4b3a11() { return T("Count as 100%", "احتسابها 100%"); }
inline QString tr_data_breakdown_b66bb7() { return T("Data breakdown", "\u062A\u0641\u0627\u0635\u064A\u0644 \u0627\u0644\u0628\u064A\u0627\u0646\u0627\u062A"); }
inline QString tr_data_entry_a353ce() { return T("  ⊞  Data Entry  ", "  ⊞  إدخال البيانات  "); }
inline QString tr_data_entry_e7b5c0() { return T("Data Entry", "إدخال البيانات"); }
inline QString tr_data_imported_successfully_c05a52() { return T("Data imported successfully.", "تم استيراد البيانات بنجاح."); }
inline QString tr_data_saved_successfully_f941c7() { return T("Data saved successfully.", "تم حفظ البيانات بنجاح."); }
inline QString tr_data_warnings_exist_e01dcc() { return T("Data warnings exist", "يوجد تحذيرات في البيانات"); }
inline QString tr_decreasing_606062() { return T("Decreasing", "الانخفاض"); }
inline QString tr_decreasing_b4c279() { return T("↘ Decreasing", "↘ انخفاض"); }
inline QString tr_decreasing_d64136() { return T("Decreasing", "انخفاض"); }
inline QString tr_descending_d6045b() { return T("Descending", "تنازلي"); }
inline QString tr_deselect_all_474bc1() { return T("✗  Deselect All", "✗  إلغاء الكل"); }
inline QString tr_deselect_all_5e3e31() { return T("Deselect all", "إلغاء التحديد"); }
inline QString tr_drag_to_reorder_right_click_a__b70e11() { return T("Drag to reorder. Right-click a chart to hide it.", "اسحب للترتيب. انقر بالزر الأيمن لإخفاء الرسم."); }
inline QString tr_duplicate_47648b() { return T("Duplicate", "تكرار"); }
inline QString tr_duplicate_account_e404c5() { return T("Duplicate account", "حساب مكرر"); }
inline QString tr_each_month_appears_as_a_dragga_9d0352() { return T("Each month appears as a draggable summary card with net sales, COGS, and trading result.", "كل شهر يظهر كبطاقة ملخص قابلة للسحب مع صافي المبيعات وتكلفة البضاعة ونتيجة التداول"); }
inline QString tr_each_month_is_shown_as_an_indi_81dc38() { return T("Each month is shown as an individual summary card.", "كل شهر يظهر ببطاقة مستقلة"); }
inline QString tr_edit_chart_9932e2() { return T("Edit chart", "تعديل الرسم"); }
inline QString tr_enter_monthly_figures_below_cl_e7d622() { return T("Enter monthly figures below. Click a month to expand it.", "أدخل الأرقام الشهرية أدناه. انقر على الشهر لتوسيعه."); }
inline QString tr_error_5a0bc4() { return T("Error", "\u062E\u0637\u0623"); }
inline QString tr_excel_workbook_xlsx_all_files_aa27cc() { return T("Excel Workbook (*.xlsx);;All Files (*)", "Excel Workbook (*.xlsx);;All Files (*)"); }
inline QString tr_expense_account_b36f13() { return T("Expense account", "حساب المصروف"); }
inline QString tr_expense_accounts_ranked_34d3f9() { return T("Expense accounts (ranked):", "\u062D\u0633\u0627\u0628\u0627\u062A \u0627\u0644\u0645\u0635\u0631\u0648\u0641 (\u0645\u0631\u062A\u0628\u0629):"); }
inline QString tr_expense_amount_1ad3d5() { return T("Expense Amount", "\u0645\u0628\u0644\u063A \u0627\u0644\u0645\u0635\u0631\u0648\u0641"); }
inline QString tr_expenses_13597e() { return T("Expenses", "\u0627\u0644\u0645\u0635\u0631\u0648\u0641\u0627\u062A"); }
inline QString tr_expenses_5a0c3c() { return T("Expenses", "مصروفات"); }
inline QString tr_expenses_ed49c9() { return T("Expenses", "\u0645\u0635\u0631\u0648\u0641\u0627\u062a"); }
inline QString tr_export_complete_fc6d0a() { return T("Export complete", "اكتمل التصدير"); }
inline QString tr_export_pdf_2cc36e() { return T("Export PDF", "تصدير PDF"); }
inline QString tr_export_pdf_b87c01() { return T("⬇  Export PDF", "\u2B07  \u062A\u0635\u062F\u064A\u0631 PDF"); }
inline QString tr_export_to_pdf_bc0791() { return T("Export to PDF", "تصدير إلى PDF"); }
inline QString tr_exported_as_a_detailed_static__365ac4() { return T("Exported as a detailed static chart", "\u062A\u0645 \u062A\u0635\u062F\u064A\u0631\u0647 \u0643\u0631\u0633\u0645 \u0633\u0627\u0628\u062A \u0645\u0641\u0635\u0644"); }
inline QString tr_extra_large_16px_3e8432() { return T("Extra Large (16px)", "\u0623\u0643\u0628\u0631 (16px)"); }
inline QString tr_failed_to_export_pdf_e10045() { return T("Failed to export PDF.", "\u0641\u0634\u0644 \u062A\u0635\u062F\u064A\u0631 PDF."); }
inline QString tr_first_period_8d67c2() { return T("First period", "\u0627\u0644\u0641\u062a\u0631\u0629 \u0627\u0644\u0623\u0648\u0644\u0649"); }
inline QString tr_group_by_835ff3() { return T("Group by", "تجميع حسب"); }
inline QString tr_grouped_bar_82dd84() { return T("Grouped bar", "أعمدة مجمعة"); }
inline QString tr_hidden_charts_7e1497() { return T("Hidden charts", "الرسوم المخفية"); }
inline QString tr_hidden_charts_e4bae7() { return T("Hidden charts", "\u0627\u0644\u0631\u0633\u0648\u0645 \u0627\u0644\u0645\u062E\u0641\u064A\u0629"); }
inline QString tr_hide_chart_9ad941() { return T("Hide chart", "إخفاء الرسم"); }
inline QString tr_high_5ed23d() { return T("High", "\u0627\u0644\u0623\u0639\u0644\u0649"); }
inline QString tr_import_data_8de4db() { return T("Import data", "استيراد البيانات"); }
inline QString tr_import_data_fbe7a5() { return T("📂  Import Data", "📂  استيراد البيانات"); }
inline QString tr_include_this_chart_in_the_resu_2a37da() { return T("Include this chart in the results", "إظهار هذا المخطط في النتائج"); }
inline QString tr_increasing_c5cd67() { return T("↗ Increasing", "↗ ارتفاع"); }
inline QString tr_increasing_da37b2() { return T("Increasing", "الارتفاع"); }
inline QString tr_increasing_faa4d2() { return T("Increasing", "ارتفاع"); }
inline QString tr_inventory_18734c() { return T("Inventory", "مخزون"); }
inline QString tr_inventory_22ffe2() { return T("Inventory", "المخزون"); }
inline QString tr_inventory_closing_d69943() { return T("Inventory Closing", "\u0627\u0644\u0645\u062E\u0632\u0648\u0646 \u0627\u0644\u062E\u062A\u0627\u0645\u064A"); }
inline QString tr_inventory_d636e9() { return T("Inventory", "\u0645\u062e\u0632\u0648\u0646"); }
inline QString tr_inventory_f08e08() { return T("Inventory", "\u0627\u0644\u0645\u062e\u0632\u0648\u0646"); }
inline QString tr_inventory_f1213f() { return T("Inventory", "\u0627\u0644\u0645\u062E\u0632\u0648\u0646"); }
inline QString tr_inventory_opening_ccde20() { return T("Inventory Opening", "\u0627\u0644\u0645\u062E\u0632\u0648\u0646 \u0627\u0644\u0627\u0641\u062A\u062A\u0627\u062D\u064A"); }
inline QString tr_iraqi_dinar_iqd_c97fa1() { return T("Iraqi Dinar (IQD)", "\u062f\u064a\u0646\u0627\u0631 \u0639\u0631\u0627\u0642\u064a (IQD)"); }
inline QString tr_label_cd5fe4() { return T("Label", "\u0627\u0644\u0639\u0646\u0648\u0627\u0646"); }
inline QString tr_landscape_94f6c5() { return T("Landscape", "أفقي"); }
inline QString tr_language_31ad2d() { return T("Language", "\u0627\u0644\u0644\u063a\u0629"); }
inline QString tr_large_14px_57768d() { return T("Large (14px)", "\u0643\u0628\u064a\u0631 (14px)"); }
inline QString tr_last_period_30676e() { return T("Last period", "\u0627\u0644\u0641\u062a\u0631\u0629 \u0627\u0644\u0623\u062e\u064a\u0631\u0629"); }
inline QString tr_left_metric_c474b3() { return T("Left metric", "المقياس الأيسر"); }
inline QString tr_light_mode_1a03c6() { return T("Light Mode", "\u0627\u0644\u0648\u0636\u0639 \u0627\u0644\u0641\u0627\u062a\u062d"); }
inline QString tr_line_133e6e() { return T("Line", "خط"); }
inline QString tr_line_a566e8() { return T("Line", "خطي"); }
inline QString tr_line_chart_932796() { return T("Line chart", "مخطط خطي"); }
inline QString tr_low_abc4e2() { return T("Low", "\u0627\u0644\u0623\u062F\u0646\u0649"); }
inline QString tr_max_gap_fd508b() { return T("Max gap", "\u0623\u0643\u0628\u0631 \u0641\u0627\u0631\u0642"); }
inline QString tr_metric_charts_eb2569() { return T("Metric charts", "مخططات المقاييس"); }
inline QString tr_month_1_5fc620() { return T("Month: %1", "الشهر: %1"); }
inline QString tr_month_460756() { return T("Month", "الشهر"); }
inline QString tr_month_9a21b7() { return T("Month", "\u0627\u0644\u0634\u0647\u0631"); }
inline QString tr_monthly_report_18dfcd() { return T("Monthly Report", "التقرير الشهري"); }
inline QString tr_monthly_report_cards_0c6d88() { return T("Monthly report cards", "بطاقات التقرير الشهري"); }
inline QString tr_months_1_b69e08() { return T("Months: %1", "الأشهر: %1"); }
inline QString tr_months_all_9f9e09() { return T("Months: All", "الأشهر: الكل"); }
inline QString tr_months_d113f0() { return T("Months: ", "الأشهر: "); }
inline QString tr_months_none_selected_7918be() { return T("Months: None selected", "الأشهر: لا شيء محدد"); }
inline QString tr_negative_profit_margin_loss_de_87719f() { return T("Negative trading result (Loss) detected.", "تم رصد نتيجة تداول سلبية (خسارة)."); }
inline QString tr_net_sales_23a2f1() { return T("Net Sales", "صافي المبيعات"); }
inline QString tr_net_sales_90f56d() { return T("Net Sales", "\u0635\u0627\u0641\u064A \u0627\u0644\u0645\u0628\u064A\u0639\u0627\u062A"); }
inline QString tr_net_sales_ae3003() { return T("Net Sales", "\u0635\u0627\u0641\u064a \u0627\u0644\u0645\u0628\u064a\u0639\u0627\u062a"); }
inline QString tr_net_sales_e81e65() { return T("NET SALES", "صافي المبيعات"); }
inline QString tr_no_accounts_added_yet_b3f1b8() { return T("No accounts added yet.", "لم تتم إضافة حسابات بعد."); }
inline QString tr_no_accounts_match_the_selected_a3e66e() { return T("No accounts match the selected group.", "لا توجد حسابات ضمن هذا التجميع."); }
inline QString tr_no_charts_selected_7a4c8f() { return T("No charts selected.", "لا توجد رسوم مختارة"); }
inline QString tr_no_months_available_9220b0() { return T("No months available.", "لا توجد أشهر متاحة"); }
inline QString tr_no_results_available_669e79() { return T("No results available.", "لا توجد نتائج متاحة"); }
inline QString tr_normal_12px_ac2426() { return T("Normal (12px)", "\u0639\u0627\u062f\u064a (12px)"); }
inline QString tr_opening_stock_first_period_ba1057() { return T("Opening Stock (First Period)", "اول المدة"); }
inline QString tr_page_1_d40a68() { return T("Page: %1", "الصفحة: %1"); }
inline QString tr_page_break_0e9502() { return T("Page break", "فاصل صفحة"); }
inline QString tr_page_separator_5ac5db() { return T("Page separator", "فاصل صفحة"); }
inline QString tr_paym_631d4e() { return T("Paym.", "\u062f\u0641\u0639"); }
inline QString tr_pie_97ce50() { return T("Pie", "دائري"); }
inline QString tr_pie_chart_9d4e04() { return T("Pie chart", "مخطط دائري"); }
inline QString tr_please_calculate_first_then_ex_3d96fc() { return T("Please calculate first, then export.", "يرجى الحساب أولاً ثم التصدير."); }
inline QString tr_portrait_247c2f() { return T("Portrait", "عمودي"); }
inline QString tr_profit_margin_56b595() { return T("Trading Result", "نتيجة التداول"); }
inline QString tr_profit_margin_dafda2() { return T("TRADING RESULT", "نتيجة التداول"); }
inline QString tr_profit_margin_ec3b22() { return T("Trading Result", "نتيجة التداول"); }
inline QString tr_profit_margin_ff57d3() { return T("Trading Result", "نتيجة التداول"); }
inline QString tr_purch_1e85b9() { return T("Purch.", "\u0634\u0631\u0627\u0621"); }
inline QString tr_purchases_00c2b6() { return T("Purchases", "\u0627\u0644\u0645\u0634\u062a\u0631\u064a\u0627\u062a"); }
inline QString tr_purchases_16236c() { return T("Purchases", "مشتريات"); }
inline QString tr_purchases_513aec() { return T("Purchases", "\u0627\u0644\u0645\u0634\u062A\u0631\u064A\u0627\u062A"); }
inline QString tr_purchases_988898() { return T("Purchases", "\u0645\u0634\u062a\u0631\u064a\u0627\u062a"); }
inline QString tr_remove_c3a712() { return T("Remove", "حذف"); }
inline QString tr_remove_page_separator_f78ac7() { return T("Remove page separator", "إزالة فاصل الصفحة"); }
inline QString tr_results_87ae7f() { return T("  ◈  Results  ", "  ◈  النتائج  "); }
inline QString tr_results_flow_77d465() { return T("Results flow", "تدفق النتائج"); }
inline QString tr_results_page_3159bf() { return T("Results page", "صفحة النتائج"); }
inline QString tr_right_metric_1d236d() { return T("Right metric", "المقياس الأيمن"); }
inline QString tr_sales_4af850() { return T("Sales", "المبيعات"); }
inline QString tr_sales_4fd176() { return T("Sales", "\u0627\u0644\u0645\u0628\u064a\u0639\u0627\u062a"); }
inline QString tr_sales_8fb4a6() { return T("Sales", "\u0627\u0644\u0645\u0628\u064A\u0639\u0627\u062A"); }
inline QString tr_sales_amount_0a3f3e() { return T("Sales Amount", "مبلغ المبيعات"); }
inline QString tr_sales_return_08f992() { return T("Sales Return", "\u0645\u0631\u062A\u062C\u0639\u0627\u062A \u0627\u0644\u0645\u0628\u064A\u0639\u0627\u062A"); }
inline QString tr_sales_return_27c2fd() { return T("Sales Return", "مردودات المبيعات"); }
inline QString tr_sales_return_7b335a() { return T("Sales Return", "\u0645\u0631\u062a\u062c\u0639\u0627\u062a"); }
inline QString tr_sales_return_e520e9() { return T("Sales Return", "مرتجعات"); }
inline QString tr_sales_revenue_41bc0b() { return T("Sales & Revenue", "المبيعات والإيرادات"); }
inline QString tr_save_data_e6059e() { return T("💾  Save Data", "💾  حفظ البيانات"); }
inline QString tr_save_data_ee42b8() { return T("Save data", "حفظ البيانات"); }
inline QString tr_select_all_48e265() { return T("Select all", "تحديد الكل"); }
inline QString tr_select_all_7812c3() { return T("✓  Select All", "✓  تحديد الكل"); }
inline QString tr_select_charts_d37b65() { return T("Select Charts", "اختيار المخططات"); }
inline QString tr_series_a_2b8d21() { return T("Series A", "السلسلة الأولى"); }
inline QString tr_series_b_b63de0() { return T("Series B", "السلسلة الثانية"); }
inline QString tr_settings_a33f70() { return T("Settings", "\u0625\u0639\u062f\u0627\u062f\u0627\u062a \u0627\u0644\u062a\u0637\u0628\u064a\u0642"); }
inline QString tr_settings_a53cf0() { return T("Settings", "\u0627\u0644\u0625\u0639\u062f\u0627\u062f\u0627\u062a"); }
inline QString tr_settings_b7a402() { return T("⚙  Settings", "\u2699  \u0625\u0639\u062F\u0627\u062F\u0627\u062A"); }
inline QString tr_show_9ed617() { return T("Show", "عرض"); }
inline QString tr_show_graphs_26cf20() { return T("Show graphs", "عرض الرسوم"); }
inline QString tr_show_results_2b2ce7() { return T("▶  Show Results", "▶  عرض النتائج"); }
inline QString tr_sort_0f56a7() { return T("Sort", "فرز"); }
inline QString tr_summary_04237c() { return T("Summary", "ملخص"); }
inline QString tr_supplier_payments_bb713e() { return T("Supplier Payments", "\u062F\u0641\u0639\u0627\u062A \u0627\u0644\u0645\u0648\u0631\u062F\u064A\u0646"); }
inline QString tr_supplier_payments_eeef31() { return T("Supplier Payments", "مدفوعات الموردين"); }
inline QString tr_supplier_purchases_f5a1cd() { return T("Supplier Purchases", "مشتريات الموردين"); }
inline QString tr_suppliers_7beff3() { return T("Suppliers", "الموردون"); }
inline QString tr_supplier_name_5c7e41() { return T("Supplier Name", "اسم المورد"); }
inline QString tr_periodic_inventory_8a4f19() { return T("Periodic Inventory", "الجرد الدوري"); }
inline QString tr_ongoing_inventory_4f9f2c() { return T("Ongoing Inventory", "الجرد المستمر"); }
inline QString tr_cogs_input_2a1b7e() { return T("COGS", "تكلفة البضاعة"); }
inline QString tr_net_sales_ongoing_b63f87() { return T("Net Sales", "صافي المبيعات"); }
inline QString tr_profit_margin_ongoing_fdb18a() { return T("Trading Result", "نتيجة التداول"); }
inline QString tr_switch_between_the_card_based__d92db3() { return T("Switch between the card-based input view (default) and the classic spreadsheet table.", "\u0627\u0644\u062a\u0628\u062f\u064a\u0644 \u0628\u064a\u0646 \u0639\u0631\u0636 \u0627\u0644\u0628\u0637\u0627\u0642\u0627\u062a \u0648\u0627\u0644\u062c\u062f\u0648\u0644 \u0627\u0644\u0643\u0644\u0627\u0633\u064a\u0643\u064a."); }
inline QString tr_text_size_5b8d4f() { return T("Text Size:", "\u062d\u062c\u0645 \u0627\u0644\u062e\u0637:"); }
inline QString tr_the_pdf_report_was_exported_su_37f5d4() { return T("The PDF report was exported successfully.", "تم تصدير تقرير PDF بنجاح."); }
inline QString tr_the_report_shows_each_month_fi_883ac5() { return T("The report shows each month first, then the charts you select below.", "التقرير يعرض كل شهر أولاً ثم المخططات التي تختارها أدناه"); }
inline QString tr_this_will_erase_all_entered_da_382bea() { return T("⚠️  This will erase all entered data for all 12 months.\n\nAre you sure you want to continue?", "⚠️  سيتم حذف جميع البيانات المدخلة للأشهر الاثني عشر.\n\nهل أنت متأكد؟"); }
inline QString tr_timeline_22644f() { return T("Timeline", "الزمن"); }
inline QString tr_timeline_7ab9a0() { return T("Timeline", "\u0627\u0644\u0632\u0645\u0646"); }
inline QString tr_title_c1c427() { return T("Title", "العنوان"); }
inline QString tr_total_a52764() { return T("Total", "\u0627\u0644\u0625\u062C\u0645\u0627\u0644\u064A"); }
inline QString tr_remaining_1f3b2a() { return T("Remaining", "\u0627\u0644\u0645\u062A\u0628\u0642\u064A"); }
inline QString tr_unable_to_read_the_xlsx_file_cd99e7() { return T("Unable to read the XLSX file.", "تعذر قراءة ملف XLSX."); }
inline QString tr_unable_to_write_the_xlsx_file_da2b9b() { return T("Unable to write the XLSX file.", "تعذر كتابة ملف XLSX."); }
inline QString tr_unknown_0240b2() { return T("Unknown", "\u063A\u064A\u0631 \u0645\u0639\u0631\u0648\u0641"); }
inline QString tr_unusual_stock_increase_closing_97f885() { return T("Unusual stock increase: Closing inventory is greater than opening inventory + purchases.", "زيادة غير معتادة في المخزون: مخزون اخر المدة أكبر من مخزون اول المدة + المشتريات."); }inline QString tr_us_dollar_105f33() { return T("US Dollar ($)", "\u062f\u0648\u0644\u0627\u0631 \u0623\u0645\u0631\u064a\u0643\u064a ($)"); }
inline QString tr_value_dbccfd() { return T("Value", "\u0627\u0644\u0642\u064A\u0645\u0629"); }
inline QString tr_warnings_5eb706() { return T("Warnings", "تحذيرات"); }
inline QString tr_yes_abc123() { return T("Yes", "نعم"); }
inline QString tr_no_def456() { return T("No", "لا"); }
inline QString tr_english_7e4a3f() { return T("English", "الإنجليزية"); }
inline QString tr_arabic_41c9d8() { return T("Arabic", "العربية"); }
inline QString tr_vs_6f0b2a() { return T(" vs ", " مقابل "); }
inline QString tr_account_assistant_report_4d9a7e() { return T("Accounting Report", "تقرير محاسبي"); }
inline QString tr_pdf_file_filter_8e3d1c() { return T("PDF (*.pdf)", "PDF (*.pdf)"); }
inline QString tr_default_account_data_filename_0aa2f1() { return T("AccountData_%1.xlsx", "AccountData_%1.xlsx"); }
inline QString tr_default_account_report_filename_6c5a9b() { return T("AccountReport_%1.pdf", "AccountReport_%1.pdf"); }

inline QStringList monthNames()
{
    if (g_lang == AppLanguage::English) {
        return {
            QStringLiteral("January"),
            QStringLiteral("February"),
            QStringLiteral("March"),
            QStringLiteral("April"),
            QStringLiteral("May"),
            QStringLiteral("June"),
            QStringLiteral("July"),
            QStringLiteral("August"),
            QStringLiteral("September"),
            QStringLiteral("October"),
            QStringLiteral("November"),
            QStringLiteral("December")
        };
    }
    return {
        QStringLiteral("يناير"),
        QStringLiteral("فبراير"),
        QStringLiteral("مارس"),
        QStringLiteral("أبريل"),
        QStringLiteral("مايو"),
        QStringLiteral("يونيو"),
        QStringLiteral("يوليو"),
        QStringLiteral("أغسطس"),
        QStringLiteral("سبتمبر"),
        QStringLiteral("أكتوبر"),
        QStringLiteral("نوفمبر"),
        QStringLiteral("ديسمبر")
    };
}
// Centralized translations moved from UI/source files.
inline QString tr_auto_clear_results_118d7c() { return T("Clear Results", "\u0645\u0633\u062D \u0627\u0644\u0646\u062A\u0627\u0626\u062C"); }
inline QString tr_auto_clear_results_warning_778aa1() { return T("This will clear all result numbers and all charts. Are you sure you want to continue?", "\u0633\u064A\u062A\u0645 \u0645\u0633\u062D \u062C\u0645\u064A\u0639 \u0623\u0631\u0642\u0627\u0645 \u0627\u0644\u0646\u062A\u0627\u0626\u062C \u0648\u062C\u0645\u064A\u0639 \u0627\u0644\u0631\u0633\u0648\u0645. \u0647\u0644 \u0623\u0646\u062A \u0645\u062A\u0623\u0643\u062F \u0623\u0646\u0643 \u062A\u0631\u064A\u062F \u0627\u0644\u0645\u062A\u0627\u0628\u0639\u0629\u061F"); }
inline QString tr_auto_summary_7c91cb2b() { return T("Summary", "الملخص"); }
inline QString tr_auto_add_a_summary_total_at_the_end_of_the_grap_fc422aba() { return T("Add a summary total at the end of the graph.", "إضافة مجموع ملخص في نهاية الرسم البياني."); }
inline QString tr_auto_graph_metrics_b363616c() { return T("Graph metrics", "مقاييس الرسم"); }
inline QString tr_auto_supplier_name_ac45e726() { return T("Supplier name", "اسم المورد"); }
inline QString tr_auto_previous_balance_d6da85a6() { return T("Previous balance", "الرصيد السابق"); }
inline QString tr_auto_purchases_eb5647b3() { return T("Purchases", "المشتريات"); }
inline QString tr_auto_total_debt_b9772183() { return T("Total debt", "إجمالي الدين"); }
inline QString tr_auto_payments_726d1e53() { return T("Payments", "الدفعات"); }
inline QString tr_auto_payment_of_purchases_81a9c0e3() { return T("Payment % of purchases", "نسبة الدفع من المشتريات"); }
inline QString tr_auto_payment_of_debt_ba7e4d60() { return T("Payment % of debt", "نسبة الدفع من الدين"); }
inline QString tr_auto_supplier_balance_74852681() { return T("Supplier balance", "رصيد المورد"); }
inline QString tr_auto_remove_supplier_e5807211() { return T("Remove supplier", "حذف المورد"); }
inline QString tr_auto_add_supplier_58130448() { return T("+ Add supplier", "+ إضافة مورد"); }
inline QString tr_auto_payment_of_monthly_purchases_9baf921e() { return T("Payment % of monthly purchases", "نسبة الدفع من مشتريات الشهر"); }
inline QString tr_auto_payment_of_total_debt_444c6cc5() { return T("Payment % of total debt", "نسبة الدفع من إجمالي الدين"); }
inline QString tr_auto_suppliers_7beff393() { return T("Suppliers", "الموردون"); }
inline QString tr_auto_each_row_tracks_one_supplier_across_all_mo_c594a14b() { return T("Each row tracks one supplier across all months. Previous balance rolls forward automatically.", "كل صف يتابع مورداً واحداً عبر جميع الأشهر. يتم ترحيل الرصيد السابق تلقائياً."); }
inline QString tr_auto_supplier_previous_balance_bd51822f() { return T("Supplier previous balance", "الرصيد السابق للمورد"); }
inline QString tr_auto_supplier_total_debt_26cc7be2() { return T("Supplier total debt", "إجمالي دين المورد"); }
inline QString tr_auto_supplier_payment_of_purchases_76125d4c() { return T("Supplier payment % of purchases", "نسبة دفع المورد من المشتريات"); }
inline QString tr_auto_supplier_payment_of_debt_869d769d() { return T("Supplier payment % of debt", "نسبة دفع المورد من الدين"); }
inline QString tr_auto_import_failed_the_workbook_format_does_not_ac2d8f1c() { return T("Import failed: the workbook format does not match the exported template.", "فشل الاستيراد: تنسيق ملف العمل لا يطابق قالب التصدير."); }
inline QString tr_auto_import_error_a_numeric_field_contains_text_ba89d1c8() { return T("Import error: a numeric field contains text.", "خطأ في الاستيراد: أحد الحقول الرقمية يحتوي على نص."); }
inline QString tr_auto_import_failed_the_data_entry_sheet_does_no_57ac18f5() { return T("Import failed: the DATA_ENTRY sheet does not match the exported template.", "فشل الاستيراد: ورقة إدخال البيانات لا تطابق قالب التصدير."); }
inline QString tr_auto_import_failed_the_expenses_sheet_does_not__1996b8cb() { return T("Import failed: the EXPENSES sheet does not match the exported template.", "فشل الاستيراد: ورقة المصروفات لا تطابق قالب التصدير."); }
inline QString tr_auto_import_failed_the_suppliers_sheet_does_not_c5805d00() { return T("Import failed: the SUPPLIERS sheet does not match the exported template.", "فشل الاستيراد: ورقة الموردين لا تطابق قالب التصدير."); }
inline QString tr_auto_missing_sheet_marker_or_headers_b00ed7c6() { return T("Missing sheet marker or headers.", "ملف الاستيراد لا يحتوي على فهرس أو عناوين."); }
inline QString tr_auto_switch_inventory_mode_1fb086f6() { return T("Switch inventory mode", "تبديل وضع الجرد"); }
inline QString tr_auto_switching_the_inventory_mode_will_clear_th_9988b06e() { return T("Switching the inventory mode will clear the current data.", "تبديل وضع الجرد سيؤدي إلى مسح البيانات الحالية."); }
inline QString tr_auto_choose_how_to_continue_1e9d6832() { return T("Choose how to continue.", "اختر كيف تريد المتابعة."); }
inline QString tr_auto_clear_them_c4858318() { return T("Clear them", "مسحها"); }
inline QString tr_auto_save_data_and_clear_91f861f8() { return T("Save data and clear", "حفظ البيانات ومسحها"); }
inline QString tr_auto_cancel_8d40ef3e() { return T("Cancel", "إلغاء"); }
inline QString tr_auto_choose_what_to_export_a3c74f3f() { return T("Choose what to export.", "اختر ما تريد تصديره."); }
inline QString tr_auto_current_tab_341e1745() { return T("Current tab", "التبويب الحالي"); }
inline QString tr_auto_all_data_2e629e38() { return T("All data", "كل البيانات"); }
inline QString tr_auto_the_workbook_did_not_contain_any_importabl_5d9e29f7() { return T("The workbook did not contain any importable rows.", "ملف العمل لا يحتوي على صفوف قابلة للاستيراد."); }
inline QString tr_auto_one_or_more_expense_accounts_already_exist_ecc8b7fa() { return T("One or more expense accounts already exist with different data. Press I know to replace them, or Cancel to stop the import.", "يوجد حساب مصروفات واحد أو أكثر بنفس الاسم لكن ببيانات مختلفة. اضغط أعرف لاستبدالها، أو إلغاء لإيقاف الاستيراد."); }
inline QString tr_auto_i_know_96d277a3() { return T("I know", "أعرف"); }
inline QString tr_import_account_type_receivable_ar_keyword() { return QString::fromUtf8("مدين"); }
